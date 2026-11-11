//! gen-stubs: generuje szkielet logujący obiektów COM WebView2 w C (styl Wine).
//!
//! Wejście:
//!   - nagłówek wygenerowany przez widl z webview2.idl (dokładne sygnatury C, łańcuchy
//!     dziedziczenia z grup `/*** X methods ***/` w strukturach `XVtbl`),
//!   - sam webview2.idl (kierunek parametrów `[out]`, którego nagłówek nie zachowuje).
//! Wyjście: stubs.c + stubs.h.
//!
//! Każda „rodzina” interfejsów implementowanych przez runtime (np. ICoreWebView2 … ICoreWebView2_29)
//! dostaje jeden obiekt z vtable najnowszej wersji; QueryInterface zna cały łańcuch.
//! Każda metoda loguje wywołanie (FIXME) i zwraca S_OK z wyzerowanymi parametrami wyjściowymi;
//! parametry wyjściowe będące interfejsami runtime'u dostają nowy obiekt-zaślepkę, żeby aplikacja
//! szła dalej i odsłaniała kolejne używane metody.
//!
//! Użycie: gen-stubs <webview2.h> <webview2.idl> <katalog_wyjściowy> [<api_gen.rs>]
//! Z czwartym argumentem generuje też ogólnego pośrednika (proxy.rs, docs/architektura-proxy.md).

mod proxy;

use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fmt::Write as _;
use std::{env, fs, process};

#[derive(Debug, Clone)]
struct Param {
    ty: String,
    name: String,
}

#[derive(Debug, Clone)]
struct Method {
    ret: String,
    name: String,
    params: Vec<Param>,
}

#[derive(Debug)]
struct Group {
    declaring: String,
    methods: Vec<Method>,
}

#[derive(Debug)]
struct Iface {
    name: String,
    groups: Vec<Group>,
}

fn die(msg: &str) -> ! {
    eprintln!("gen-stubs: {msg}");
    process::exit(1)
}

/// Dzieli po przecinkach na najwyższym poziomie zagnieżdżenia () i [].
fn split_top(s: &str, sep: char) -> Vec<String> {
    let mut out = Vec::new();
    let mut depth = 0i32;
    let mut cur = String::new();
    for c in s.chars() {
        match c {
            '(' | '[' => depth += 1,
            ')' | ']' => depth -= 1,
            _ => {}
        }
        if c == sep && depth == 0 {
            out.push(cur.trim().to_string());
            cur.clear();
        } else {
            cur.push(c);
        }
    }
    if !cur.trim().is_empty() {
        out.push(cur.trim().to_string());
    }
    out
}

/// "const WCHAR *foo" -> (typ "const WCHAR *", nazwa "foo")
fn split_param(p: &str) -> Param {
    let p = p.split_whitespace().collect::<Vec<_>>().join(" ");
    let idx = p
        .rfind(|c: char| !(c.is_alphanumeric() || c == '_'))
        .map(|i| i + 1)
        .unwrap_or(0);
    let (ty, name) = p.split_at(idx);
    Param { ty: ty.trim().to_string(), name: name.to_string() }
}

fn strip_comments(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let b = s.as_bytes();
    let mut i = 0;
    while i < b.len() {
        if b[i] == b'/' && i + 1 < b.len() && b[i + 1] == b'/' {
            while i < b.len() && b[i] != b'\n' {
                i += 1;
            }
        } else if b[i] == b'/' && i + 1 < b.len() && b[i + 1] == b'*' {
            i += 2;
            while i + 1 < b.len() && !(b[i] == b'*' && b[i + 1] == b'/') {
                i += 1;
            }
            i += 2;
        } else {
            out.push(b[i] as char);
            i += 1;
        }
    }
    out
}

/// Parsuje struktury `typedef struct XVtbl { ... } XVtbl;` z nagłówka widl (część C, nie C++).
fn parse_header(h: &str) -> (Vec<Iface>, BTreeSet<String>) {
    let mut ifaces = Vec::new();
    let mut structs = BTreeSet::new();
    for line in h.lines() {
        if let Some(rest) = line.strip_prefix("typedef struct ") {
            let n: String = rest.chars().take_while(|c| c.is_alphanumeric() || *c == '_').collect();
            if !n.ends_with("Vtbl") && rest.trim_end().ends_with('{') {
                structs.insert(n);
            }
        }
    }
    let mut pos = 0;
    while let Some(off) = h[pos..].find("typedef struct ") {
        let start = pos + off + "typedef struct ".len();
        let name: String = h[start..].chars().take_while(|c| c.is_alphanumeric() || *c == '_').collect();
        pos = start;
        let Some(iname) = name.strip_suffix("Vtbl") else { continue };
        if !h[start + name.len()..].starts_with(" {") {
            continue;
        }
        let end_marker = format!("}} {name};");
        let end = h[start..].find(&end_marker).unwrap_or_else(|| die(&format!("brak końca {name}"))) + start;
        let body = &h[start..end];
        let mut groups: Vec<Group> = Vec::new();
        for chunk in body.split("/*** ").skip(1) {
            let declaring = chunk.split_whitespace().next().unwrap().to_string();
            let mut methods = Vec::new();
            for m in chunk.split(";").filter(|m| m.contains("(STDMETHODCALLTYPE *")) {
                let (head, tail) = m.split_once("(STDMETHODCALLTYPE *").unwrap();
                let ret = head.lines().last().unwrap().trim().to_string();
                let (mname, rest) = tail.split_once(")").unwrap();
                let args = rest.trim().strip_prefix('(').unwrap();
                let args = args.strip_suffix(')').unwrap_or(args);
                let mut params: Vec<Param> = split_top(args, ',').iter().map(|p| split_param(p)).collect();
                if params.is_empty() || params[0].name != "This" {
                    die(&format!("{iname}::{mname}: brak This"));
                }
                params.remove(0);
                methods.push(Method { ret, name: mname.trim().to_string(), params });
            }
            groups.push(Group { declaring, methods });
        }
        ifaces.push(Iface { name: iname.to_string(), groups });
        pos = end;
    }
    (ifaces, structs)
}

/// Z IDL: dla każdego interfejsu lista metod -> lista flag `[out]` parametrów.
fn parse_idl(idl: &str) -> HashMap<String, Vec<(String, Vec<bool>)>> {
    let src = strip_comments(idl);
    let mut out = HashMap::new();
    let mut pos = 0;
    while let Some(off) = src[pos..].find("interface ") {
        let start = pos + off + "interface ".len();
        pos = start;
        let header_end = match src[start..].find(|c| c == '{' || c == ';') {
            Some(e) => start + e,
            None => break,
        };
        if src.as_bytes()[header_end] == b';' {
            continue; // deklaracja zapowiadająca
        }
        let name: String = src[start..].chars().take_while(|c| c.is_alphanumeric() || *c == '_').collect();
        let body_end = src[header_end..].find('}').unwrap() + header_end;
        let body = &src[header_end + 1..body_end];
        let mut methods = Vec::new();
        for decl in body.split(';').map(str::trim).filter(|d| !d.is_empty()) {
            let mut d = decl;
            let mut prop = "";
            if d.starts_with('[') {
                let close = d.find(']').unwrap();
                let attrs = &d[1..close];
                if attrs.contains("propget") {
                    prop = "get_";
                } else if attrs.contains("propput") {
                    prop = "put_";
                }
                d = d[close + 1..].trim();
            }
            let d = d.strip_prefix("HRESULT").unwrap_or_else(|| die(&format!("{name}: {d}"))).trim();
            let (mname, args) = d.split_once('(').unwrap();
            let args = args.trim().strip_suffix(')').unwrap();
            let outs = split_top(args, ',')
                .iter()
                .map(|p| p.starts_with('[') && p[..p.find(']').unwrap()].contains("out"))
                .collect();
            methods.push((format!("{prop}{}", mname.trim()), outs));
        }
        out.insert(name, methods);
        pos = body_end;
    }
    out
}

/// Interfejsy implementowane przez APLIKACJĘ (nie przez runtime) - nie generujemy dla nich obiektów.
fn implemented_by_app(name: &str) -> bool {
    name.ends_with("Handler")
        || name.starts_with("ICoreWebView2EnvironmentOptions")
        || name == "ICoreWebView2CustomSchemeRegistration"
        || name == "IUnknown"
}

enum Kind {
    Str,
    Int,
    Int64,
    Float,
    Handle,
    Rect,
    Point,
    Token,
    Ptr,
    Opaque,
}

fn classify(ty: &str, structs: &BTreeSet<String>) -> Kind {
    let t = ty.replace("const ", "");
    let t = t.trim();
    match t {
        "LPCWSTR" | "LPWSTR" | "WCHAR *" => Kind::Str,
        "BOOL" | "INT32" | "int" | "UINT32" | "UINT" | "ULONG" | "LONG" | "INT" | "DWORD" | "BYTE"
        | "UINT8" | "UINT16" | "INT16" | "USHORT" | "SHORT" => Kind::Int,
        "UINT64" | "INT64" | "ULONG64" | "LONG64" | "ULONGLONG" | "LONGLONG" => Kind::Int64,
        "double" | "float" | "DOUBLE" | "FLOAT" => Kind::Float,
        "HWND" | "HANDLE" | "HCURSOR" | "HICON" | "HMODULE" | "HINSTANCE" | "WPARAM" | "LPARAM" => {
            Kind::Handle
        }
        "RECT" => Kind::Rect,
        "POINT" => Kind::Point,
        "EventRegistrationToken" => Kind::Token,
        _ if t.ends_with('*') => Kind::Ptr,
        _ if t.starts_with("COREWEBVIEW2_") && !structs.contains(t) => Kind::Int, // enum
        _ => Kind::Opaque,
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 4 && args.len() != 5 {
        die("użycie: gen-stubs <webview2.h> <webview2.idl> <katalog_wyjściowy> [<api_gen.rs>]");
    }
    let header = fs::read_to_string(&args[1]).unwrap_or_else(|e| die(&format!("{}: {e}", args[1])));
    let idl = fs::read_to_string(&args[2]).unwrap_or_else(|e| die(&format!("{}: {e}", args[2])));
    let outdir = &args[3];

    let (ifaces, structs) = parse_header(&header);
    let idl_methods = parse_idl(&idl);
    let by_name: HashMap<&str, &Iface> = ifaces.iter().map(|i| (i.name.as_str(), i)).collect();

    // Liście = interfejsy, które nie są przodkiem żadnego innego.
    let mut ancestors: BTreeSet<&str> = BTreeSet::new();
    for i in &ifaces {
        for g in &i.groups {
            if g.declaring != i.name {
                ancestors.insert(&g.declaring);
            }
        }
    }
    let leaves: Vec<&Iface> = ifaces
        .iter()
        .filter(|i| !ancestors.contains(i.name.as_str()) && !implemented_by_app(&i.name))
        .collect();

    // Interfejs -> rodzina (liść), do tworzenia obiektów dla parametrów wyjściowych.
    let mut family_of: BTreeMap<&str, &str> = BTreeMap::new();
    for l in &leaves {
        for g in &l.groups {
            if g.declaring == "IUnknown" {
                continue;
            }
            if let Some(prev) = family_of.insert(&g.declaring, &l.name) {
                die(&format!("{} należy do dwóch rodzin: {prev} i {}", g.declaring, l.name));
            }
        }
    }

    // Sprawdzenie zgodności nagłówek <-> IDL i zebranie flag [out].
    let mut out_flags: HashMap<(String, String), Vec<bool>> = HashMap::new();
    for i in &ifaces {
        let Some(own) = i.groups.iter().find(|g| g.declaring == i.name) else { continue };
        let idl_m = idl_methods.get(&i.name).unwrap_or_else(|| die(&format!("{} brak w IDL", i.name)));
        if idl_m.len() != own.methods.len() {
            die(&format!("{}: {} metod w nagłówku, {} w IDL", i.name, own.methods.len(), idl_m.len()));
        }
        for (m, (iname, flags)) in own.methods.iter().zip(idl_m) {
            if &m.name != iname || flags.len() != m.params.len() {
                die(&format!("{}: niezgodność {} / {}", i.name, m.name, iname));
            }
            out_flags.insert((i.name.clone(), m.name.clone()), flags.clone());
        }
    }

    if let Some(rust_file) = args.get(4) {
        let app_ifaces: Vec<&Iface> =
            ifaces.iter().filter(|i| implemented_by_app(&i.name) && i.name != "IUnknown").collect();
        proxy::generate(&ifaces, &out_flags, &leaves, &app_ifaces, outdir, rust_file);
        // Architektura v8: obiekty żyją w pomocniku, zaślepki C (stubs.c) nie są już potrzebne.
        return;
    }

    let mut c = String::new();
    let mut h = String::new();
    let total_methods: usize = leaves.iter().map(|l| l.groups.iter().map(|g| g.methods.len()).sum::<usize>()).sum();

    writeln!(h, "/* Wygenerowane przez tools/gen-stubs - NIE EDYTOWAĆ ręcznie. */").unwrap();
    writeln!(h, "#ifndef __WEBVIEW2_STUBS_H\n#define __WEBVIEW2_STUBS_H\n").unwrap();
    writeln!(h, "struct stub_family\n{{\n    const char *name;\n    const void *vtbl;\n    const IID *const *iids;\n}};\n").unwrap();
    writeln!(h, "/* Wspólny układ obiektu-zaślepki; `data` do użytku ręcznie napisanych nadpisań metod. */").unwrap();
    writeln!(h, "struct stub_object\n{{\n    const void *vtbl;\n    LONG ref;\n    const struct stub_family *family;\n    void *data;\n    void (*destroy)(struct stub_object *obj);\n}};\n").unwrap();
    writeln!(h, "/* Tworzy obiekt-zaślepkę rodziny; zwraca wskaźnik na interfejs (ref = 1) albo NULL. */").unwrap();
    writeln!(h, "void *stub_create(const struct stub_family *family);\n").unwrap();

    writeln!(c, "/*\n * Szkielet logujący WebView2 - wygenerowany przez tools/gen-stubs, NIE EDYTOWAĆ ręcznie.").unwrap();
    writeln!(c, " * Rodzin: {}, metod (z IUnknown): {}.\n *", leaves.len(), total_methods).unwrap();
    writeln!(c, " * Każda metoda loguje FIXME i zwraca S_OK z wyzerowanymi parametrami wyjściowymi.").unwrap();
    writeln!(c, " */\n").unwrap();
    c.push_str(PRELUDE);
    let invokers_at = c.len();
    let mut invokers: BTreeMap<String, String> = BTreeMap::new();

    for l in &leaves {
        let leaf = &l.name;
        // tablica IID
        write!(c, "\n/* ===== {leaf} ===== */\n\nstatic const IID *const {leaf}_iids[] =\n{{\n    &IID_IUnknown,\n").unwrap();
        for g in l.groups.iter().filter(|g| g.declaring != "IUnknown") {
            writeln!(c, "    &IID_{},", g.declaring).unwrap();
        }
        c.push_str("    NULL\n};\n");

        let mut vtbl_entries = vec![
            "(void *)stub_QueryInterface".to_string(),
            "(void *)stub_AddRef".to_string(),
            "(void *)stub_Release".to_string(),
        ];
        for g in l.groups.iter().filter(|g| g.declaring != "IUnknown") {
            for m in &g.methods {
                let fname = format!("{}_{}", g.declaring, m.name);
                let flags = &out_flags[&(g.declaring.clone(), m.name.clone())];
                vtbl_entries.push(fname.clone());
                gen_method(&mut c, leaf, &fname, m, flags, &structs, &family_of, &by_name, &mut invokers);
            }
        }
        write!(c, "\nstatic const {leaf}Vtbl {leaf}_stub_vtbl =\n{{\n").unwrap();
        for e in &vtbl_entries {
            writeln!(c, "    {e},").unwrap();
        }
        c.push_str("};\n");
        write!(
            c,
            "\nconst struct stub_family stub_family_{leaf} =\n{{\n    \"{leaf}\",\n    &{leaf}_stub_vtbl,\n    {leaf}_iids,\n}};\n"
        )
        .unwrap();
        writeln!(h, "extern const struct stub_family stub_family_{leaf};").unwrap();
    }
    writeln!(h, "\n#endif /* __WEBVIEW2_STUBS_H */").unwrap();
    let mut inv_code = String::from("\n/* ===== wywołania handlerów zakończenia ===== */\n");
    for code in invokers.values() {
        inv_code.push_str(code);
    }
    c.insert_str(invokers_at, &inv_code);

    fs::write(format!("{outdir}/stubs.c"), c).unwrap();
    fs::write(format!("{outdir}/stubs.h"), h).unwrap();
    eprintln!("gen-stubs: {} rodzin, {} metod -> {outdir}/stubs.[ch]", leaves.len(), total_methods);
    for l in &leaves {
        eprintln!("  {}", l.name);
    }
}

#[allow(clippy::too_many_arguments)]
fn gen_method(
    c: &mut String,
    leaf: &str,
    fname: &str,
    m: &Method,
    outs: &[bool],
    structs: &BTreeSet<String>,
    family_of: &BTreeMap<&str, &str>,
    by_name: &HashMap<&str, &Iface>,
    invokers: &mut BTreeMap<String, String>,
) {
    let mut sig = format!("{leaf} *iface");
    for p in &m.params {
        let sep = if p.ty.ends_with('*') { "" } else { " " };
        write!(sig, ", {}{sep}{}", p.ty, p.name).unwrap();
    }
    write!(c, "\nstatic {} STDMETHODCALLTYPE {fname}({sig})\n{{\n", m.ret).unwrap();

    // FIXME z parametrami wejściowymi
    let mut fmt = String::from("iface %p");
    let mut vals = String::from("iface");
    for (p, &is_out) in m.params.iter().zip(outs) {
        let (f, v) = if is_out {
            ("%p", p.name.clone())
        } else {
            match classify(&p.ty, structs) {
                Kind::Str => ("%s", format!("debugstr_w({})", p.name)),
                Kind::Int => ("%d", format!("(int){}", p.name)),
                Kind::Int64 => ("%s", format!("wine_dbgstr_longlong({})", p.name)),
                Kind::Float => ("%f", p.name.clone()),
                Kind::Handle | Kind::Ptr => ("%p", format!("(void *){}", p.name)),
                Kind::Rect => ("%s", format!("wine_dbgstr_rect(&{})", p.name)),
                Kind::Point => ("%s", format!("wine_dbgstr_point(&{})", p.name)),
                Kind::Token => ("%s", format!("wine_dbgstr_longlong({}.value)", p.name)),
                Kind::Opaque => ("{...}", String::new()),
            }
        };
        write!(fmt, ", {} {f}", p.name).unwrap();
        if !v.is_empty() {
            write!(vals, ", {v}").unwrap();
        }
    }
    writeln!(c, "    FIXME(\"{fmt} stub!\\n\", {vals});").unwrap();

    // parametry wyjściowe
    let is_add = m.name.starts_with("add_");
    for (p, &is_out) in m.params.iter().zip(outs) {
        if !is_out {
            continue;
        }
        let n = &p.name;
        let base = p.ty.strip_suffix('*').unwrap_or_else(|| die(&format!("{fname}: [out] bez wskaźnika"))).trim();
        if is_add && base == "EventRegistrationToken" {
            writeln!(c, "    if ({n}) {n}->value = stub_next_token();").unwrap();
        } else if base == "LPWSTR" {
            writeln!(c, "    if ({n}) *{n} = stub_strdup(L\"\");").unwrap();
        } else if let Some(iname) = base.strip_suffix('*').map(str::trim) {
            match family_of.get(iname) {
                Some(fam) if by_name.contains_key(iname) => {
                    writeln!(c, "    if ({n}) *{n} = stub_create(&stub_family_{fam});").unwrap()
                }
                _ => writeln!(c, "    if ({n}) *{n} = NULL;").unwrap(),
            }
        } else {
            writeln!(c, "    if ({n}) memset({n}, 0, sizeof(*{n}));").unwrap();
        }
    }
    // handlery zakończenia: wywołanie asynchroniczne z S_OK i pustym wynikiem
    for (p, &is_out) in m.params.iter().zip(outs) {
        let Some(hname) = p.ty.strip_suffix('*').map(str::trim) else { continue };
        if is_out || !hname.ends_with("CompletedHandler") {
            continue;
        }
        if !invokers.contains_key(hname) {
            let code = gen_invoker(hname, structs, family_of, by_name);
            invokers.insert(hname.to_string(), code);
        }
        writeln!(c, "    if ({n}) stub_invoke_async(invoke_{hname}, (IUnknown *){n});", n = p.name).unwrap();
    }
    let ret = if m.ret == "HRESULT" { "S_OK" } else { "0" };
    writeln!(c, "    return {ret};\n}}").unwrap();
}

/// Funkcja wołająca `handler->Invoke(S_OK, <puste wyniki>)` dla danego typu handlera.
fn gen_invoker(
    hname: &str,
    structs: &BTreeSet<String>,
    family_of: &BTreeMap<&str, &str>,
    by_name: &HashMap<&str, &Iface>,
) -> String {
    let h = by_name.get(hname).unwrap_or_else(|| die(&format!("brak handlera {hname}")));
    let inv = h
        .groups
        .iter()
        .flat_map(|g| g.methods.iter())
        .find(|m| m.name == "Invoke")
        .unwrap_or_else(|| die(&format!("{hname}: brak Invoke")));
    let mut decls = String::new();
    let mut args = String::new();
    let mut cleanup = String::new();
    for (i, p) in inv.params.iter().enumerate() {
        let a = if p.ty == "HRESULT" {
            "S_OK".to_string()
        } else {
            match classify(&p.ty, structs) {
                Kind::Str if p.name.to_lowercase().contains("json") => "L\"null\"".to_string(),
                Kind::Str => "L\"\"".to_string(),
                Kind::Int | Kind::Int64 | Kind::Float | Kind::Handle => "0".to_string(),
                Kind::Ptr => {
                    let base = p.ty.strip_suffix('*').unwrap().trim();
                    match family_of.get(base) {
                        Some(fam) => {
                            writeln!(decls, "    {base} *arg{i} = stub_create(&stub_family_{fam});").unwrap();
                            writeln!(cleanup, "    if (arg{i}) arg{i}->lpVtbl->Release(arg{i});").unwrap();
                            format!("arg{i}")
                        }
                        None => "NULL".to_string(),
                    }
                }
                Kind::Rect | Kind::Point | Kind::Token | Kind::Opaque => {
                    writeln!(decls, "    {} arg{i};", p.ty).unwrap();
                    writeln!(decls, "    memset(&arg{i}, 0, sizeof(arg{i}));").unwrap();
                    format!("arg{i}")
                }
            }
        };
        write!(args, ", {a}").unwrap();
    }
    format!(
        "\nstatic void invoke_{hname}(void *arg)\n{{\n    {hname} *handler = arg;\n{decls}\n    \
         TRACE(\"invoking {hname} %p\\n\", handler);\n    handler->lpVtbl->Invoke(handler{args});\n{cleanup}    \
         handler->lpVtbl->Release(handler);\n}}\n"
    )
}

const PRELUDE: &str = r#"#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "webview2.h"
#include "stubs.h"
#include "webview2_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

static void stub_invoke_async(void (*func)(void *arg), IUnknown *handler)
{
    handler->lpVtbl->AddRef(handler);
    if (!webview2_queue_async(func, handler)) handler->lpVtbl->Release(handler);
}

static HRESULT STDMETHODCALLTYPE stub_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct stub_object *obj = (struct stub_object *)iface;
    const IID *const *p;

    for (p = obj->family->iids; *p; p++)
    {
        if (IsEqualGUID(iid, *p))
        {
            InterlockedIncrement(&obj->ref);
            *out = iface;
            return S_OK;
        }
    }
    FIXME("%s %p: unsupported interface %s\n", obj->family->name, iface, debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE stub_AddRef(IUnknown *iface)
{
    struct stub_object *obj = (struct stub_object *)iface;
    ULONG ref = InterlockedIncrement(&obj->ref);
    TRACE("%s %p increasing refcount to %lu.\n", obj->family->name, iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE stub_Release(IUnknown *iface)
{
    struct stub_object *obj = (struct stub_object *)iface;
    ULONG ref = InterlockedDecrement(&obj->ref);
    TRACE("%s %p decreasing refcount to %lu.\n", obj->family->name, iface, ref);
    if (!ref)
    {
        if (obj->destroy) obj->destroy(obj);
        free(obj);
    }
    return ref;
}

void *stub_create(const struct stub_family *family)
{
    struct stub_object *obj;

    if (!(obj = calloc(1, sizeof(*obj)))) return NULL;
    obj->vtbl = family->vtbl;
    obj->ref = 1;
    obj->family = family;
    FIXME("created %s stub %p\n", family->name, obj);
    return obj;
}

static INT64 stub_next_token(void)
{
    static LONG64 next;
    return InterlockedIncrement64(&next);
}

static WCHAR *stub_strdup(const WCHAR *str)
{
    size_t size = (wcslen(str) + 1) * sizeof(WCHAR);
    WCHAR *ret = CoTaskMemAlloc(size);
    if (ret) memcpy(ret, str, size);
    return ret;
}
"#;
