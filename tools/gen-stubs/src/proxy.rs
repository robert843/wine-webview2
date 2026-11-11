//! Generator v2: ogólny pośrednik COM (docs/architektura-proxy.md).
//!
//! Wyjście:
//!   <c_dir>/proxy_gen.h, proxy_gen.c — identyfikatory metod/rodzin, vtable pośredników (CALL),
//!                                      dyspozytor wywołań na obiektach aplikacji (INVOKE)
//!   <rust_file> (api_gen.rs)          — te same identyfikatory, trait na rodzinę z domyślną
//!                                      implementacją (log + S_OK), dyspozytor, klienci obiektów aplikacji

use crate::{die, Iface, Method};
use std::collections::{BTreeMap, HashMap};
use std::fmt::Write as _;

/// Rodzaj parametru na drucie (to samo kodowanie po obu stronach).
#[derive(Clone, Debug, PartialEq)]
enum Pk {
    I32(String), // typ C (BOOL, UINT32, enum…)
    I64(String),
    F64,
    Str,
    Struct(&'static str),
    Handle,
    Iface(String), // nazwa interfejsu z typu (IUnknown dla ogólnych)
    Stream,
    Unsupported(String),
}

const STRUCTS: &[(&str, &str, &[(&str, &str)])] = &[
    ("POINT", "POINT", &[("x", "i32"), ("y", "i32")]),
    ("RECT", "RECT", &[("left", "i32"), ("top", "i32"), ("right", "i32"), ("bottom", "i32")]),
    ("EventRegistrationToken", "EventRegistrationToken", &[("value", "i64")]),
    ("COREWEBVIEW2_COLOR", "COREWEBVIEW2_COLOR", &[("A", "u8"), ("R", "u8"), ("G", "u8"), ("B", "u8")]),
    (
        "COREWEBVIEW2_PHYSICAL_KEY_STATUS",
        "COREWEBVIEW2_PHYSICAL_KEY_STATUS",
        &[
            ("RepeatCount", "u32"),
            ("ScanCode", "u32"),
            ("IsExtendedKey", "i32"),
            ("IsMenuKeyDown", "i32"),
            ("WasKeyDown", "i32"),
            ("IsKeyReleased", "i32"),
        ],
    ),
];

fn scalar(t: &str) -> Option<Pk> {
    match t {
        "BOOL" | "INT32" | "int" | "INT" | "UINT32" | "UINT" | "DWORD" | "HRESULT" => Some(Pk::I32(t.into())),
        "INT64" | "UINT64" => Some(Pk::I64(t.into())),
        "double" => Some(Pk::F64),
        "HWND" => Some(Pk::Handle),
        _ if t.starts_with("COREWEBVIEW2_") && !STRUCTS.iter().any(|s| s.0 == t) => Some(Pk::I32(t.into())),
        _ => STRUCTS.iter().find(|s| s.0 == t).map(|s| Pk::Struct(s.0)),
    }
}

fn iface_name(t: &str) -> Option<String> {
    let base = t.strip_suffix('*')?.trim();
    if base == "IStream" || base == "IUnknown" || base.starts_with("ICoreWebView2") {
        Some(base.to_string())
    } else {
        None
    }
}

fn classify(ty: &str, out: bool) -> Pk {
    let t = ty.replace("const ", "");
    let t = t.trim();
    if out {
        let Some(base) = t.strip_suffix('*').map(str::trim) else { return Pk::Unsupported(t.into()) };
        if base == "LPWSTR" {
            return Pk::Str;
        }
        if let Some(i) = iface_name(base) {
            return if i == "IStream" { Pk::Stream } else { Pk::Iface(i) };
        }
        return scalar(base).unwrap_or_else(|| Pk::Unsupported(t.into()));
    }
    if t == "LPCWSTR" || t == "LPWSTR" {
        return Pk::Str;
    }
    if let Some(i) = iface_name(t) {
        return if i == "IStream" { Pk::Stream } else { Pk::Iface(i) };
    }
    scalar(t).unwrap_or_else(|| Pk::Unsupported(t.into()))
}

struct PMethod<'a> {
    id: u32,
    decl: String,
    m: &'a Method,
    kinds: Vec<(Pk, bool)>, // (rodzaj, out)
}

impl PMethod<'_> {
    fn supported(&self) -> bool {
        self.m.ret == "HRESULT" && !self.kinds.iter().any(|(k, _)| matches!(k, Pk::Unsupported(_)))
    }
    fn ident(&self) -> String {
        format!("{}_{}", self.decl, self.m.name)
    }
}

fn fnv1a(data: &str) -> u64 {
    let mut h: u64 = 0xcbf29ce484222325;
    for b in data.bytes() {
        h ^= b as u64;
        h = h.wrapping_mul(0x100000001b3);
    }
    h
}

fn rust_scalar(pk: &Pk) -> &'static str {
    match pk {
        Pk::I32(t) if t == "UINT32" || t == "UINT" || t == "DWORD" => "u32",
        Pk::I32(t) if t == "BOOL" => "bool",
        Pk::I32(_) => "i32",
        Pk::I64(t) if t == "UINT64" => "u64",
        Pk::I64(_) => "i64",
        Pk::F64 => "f64",
        Pk::Handle => "u64",
        _ => unreachable!(),
    }
}

fn rust_type(pk: &Pk, out: bool) -> String {
    match pk {
        Pk::Str if out => "String".into(),
        Pk::Str => "Option<String>".into(),
        Pk::Struct(s) => (*s).into(),
        Pk::Iface(_) => "ObjRef".into(),
        Pk::Stream => "Option<Vec<u8>>".into(),
        other => rust_scalar(other).into(),
    }
}

fn rust_ident(name: &str) -> String {
    match name {
        "type" | "ref" | "match" | "self" | "move" | "fn" | "impl" | "use" | "mod" | "loop" | "box" => format!("r#{name}"),
        _ => name.to_string(),
    }
}

pub fn generate(ifaces: &[Iface], out_flags: &HashMap<(String, String), Vec<bool>>, leaves: &[&Iface],
                app_ifaces: &[&Iface], c_dir: &str, rust_file: &str) {
    // Globalne identyfikatory metod: każda (interfejs deklarujący, metoda) w kolejności nagłówka.
    let mut methods: BTreeMap<(String, String), PMethod> = BTreeMap::new();
    let mut order: Vec<(String, String)> = Vec::new();
    let mut signature = String::new();
    let mut next = 1u32;
    for i in ifaces {
        let Some(own) = i.groups.iter().find(|g| g.declaring == i.name) else { continue };
        for m in &own.methods {
            let flags = &out_flags[&(i.name.clone(), m.name.clone())];
            let kinds: Vec<(Pk, bool)> = m.params.iter().zip(flags).map(|(p, &o)| (classify(&p.ty, o), o)).collect();
            writeln!(signature, "{}::{}({})", i.name, m.name,
                     m.params.iter().zip(flags).map(|(p, o)| format!("{}{}", if *o { "out " } else { "" }, p.ty)).collect::<Vec<_>>().join(",")).unwrap();
            let key = (i.name.clone(), m.name.clone());
            methods.insert(key.clone(), PMethod { id: next, decl: i.name.clone(), m, kinds });
            order.push(key);
            next += 1;
        }
    }
    let api_hash = fnv1a(&signature);
    let family_index: HashMap<&str, usize> = leaves.iter().enumerate().map(|(n, l)| (l.name.as_str(), n)).collect();
    // interfejs -> rodzina (do domyślnych obiektów dla parametrów wyjściowych)
    let mut family_of: HashMap<&str, &str> = HashMap::new();
    for l in leaves {
        for g in &l.groups {
            family_of.insert(&g.declaring, &l.name);
        }
    }
    let chain = |i: &Iface| -> Vec<(String, String)> {
        i.groups.iter().filter(|g| g.declaring != "IUnknown")
            .flat_map(|g| g.methods.iter().map(move |m| (g.declaring.clone(), m.name.clone()))).collect()
    };
    let unsupported = methods.values().filter(|m| !m.supported()).count();
    eprintln!("gen-proxy: {} metod, {} nieprzenośnych (lokalnie E_NOTIMPL), hash API {:016x}",
              methods.len(), unsupported, api_hash);

    // ======================= C =======================
    let mut h = String::new();
    writeln!(h, "/* Wygenerowane przez tools/gen-stubs (proxy) — NIE EDYTOWAĆ ręcznie. */").unwrap();
    writeln!(h, "#ifndef __WEBVIEW2_PROXY_GEN_H\n#define __WEBVIEW2_PROXY_GEN_H\n").unwrap();
    writeln!(h, "#define WV2_API_HASH 0x{api_hash:016x}ull").unwrap();
    writeln!(h, "#define WV2_FAMILY_COUNT {}\n", leaves.len()).unwrap();
    writeln!(h, "enum wv2_family\n{{").unwrap();
    for (n, l) in leaves.iter().enumerate() {
        writeln!(h, "    FAMILY_{} = {n},", l.name).unwrap();
    }
    writeln!(h, "}};\n\nenum wv2_method\n{{").unwrap();
    for key in &order {
        let m = &methods[key];
        writeln!(h, "    METHOD_{} = {},", m.ident(), m.id).unwrap();
    }
    writeln!(h, "}};\n").unwrap();
    writeln!(h, "extern const void *const proxy_vtbls[WV2_FAMILY_COUNT];").unwrap();
    writeln!(h, "extern const IID *const *const proxy_family_iids[WV2_FAMILY_COUNT];").unwrap();
    writeln!(h, "extern const char *const proxy_family_names[WV2_FAMILY_COUNT];").unwrap();
    writeln!(h, "/* Executes a call from the helper on an application object (handler, options…). */").unwrap();
    writeln!(h, "HRESULT proxy_invoke_app(IUnknown *obj, UINT32 method, struct wv2_reader *args, struct wv2_buffer *out);").unwrap();
    writeln!(h, "\n#endif /* __WEBVIEW2_PROXY_GEN_H */").unwrap();

    let mut c = String::new();
    writeln!(c, "/* Wygenerowane przez tools/gen-stubs (proxy) — NIE EDYTOWAĆ ręcznie. */\n").unwrap();
    c.push_str("#include <stdarg.h>\n#include <stdlib.h>\n\n#define COBJMACROS\n#include \"windef.h\"\n#include \"winbase.h\"\n#include \"objbase.h\"\n#include \"webview2.h\"\n#include \"proxy.h\"\n#include \"proxy_gen.h\"\n\n#include \"wine/debug.h\"\n\nWINE_DEFAULT_DEBUG_CHANNEL(webview2);\n");

    for l in leaves {
        let leaf = &l.name;
        write!(c, "\n/* ===== {leaf} ===== */\n\nstatic const IID *const {leaf}_proxy_iids[] =\n{{\n    &IID_IUnknown,\n").unwrap();
        for g in l.groups.iter().filter(|g| g.declaring != "IUnknown") {
            writeln!(c, "    &IID_{},", g.declaring).unwrap();
        }
        c.push_str("    NULL\n};\n");
        let mut entries = vec!["(void *)proxy_QueryInterface".to_string(), "(void *)proxy_AddRef".into(), "(void *)proxy_Release".into()];
        for key in chain(l) {
            let pm = &methods[&key];
            let fname = format!("proxy_{}_{}", leaf, pm.ident());
            entries.push(fname.clone());
            gen_c_proxy(&mut c, leaf, &fname, pm);
        }
        write!(c, "\nstatic const {leaf}Vtbl {leaf}_proxy_vtbl =\n{{\n").unwrap();
        for e in &entries {
            writeln!(c, "    {e},").unwrap();
        }
        c.push_str("};\n");
    }
    c.push_str("\nconst void *const proxy_vtbls[WV2_FAMILY_COUNT] =\n{\n");
    for l in leaves {
        writeln!(c, "    &{}_proxy_vtbl,", l.name).unwrap();
    }
    c.push_str("};\n\nconst IID *const *const proxy_family_iids[WV2_FAMILY_COUNT] =\n{\n");
    for l in leaves {
        writeln!(c, "    {}_proxy_iids,", l.name).unwrap();
    }
    c.push_str("};\n\nconst char *const proxy_family_names[WV2_FAMILY_COUNT] =\n{\n");
    for l in leaves {
        writeln!(c, "    \"{}\",", l.name).unwrap();
    }
    c.push_str("};\n");

    // Dyspozytor INVOKE: metody interfejsów implementowanych przez aplikację.
    c.push_str("\nHRESULT proxy_invoke_app(IUnknown *obj, UINT32 method, struct wv2_reader *args, struct wv2_buffer *out)\n{\n    HRESULT hr;\n\n    switch (method)\n    {\n");
    for a in app_ifaces {
        let Some(own) = a.groups.iter().find(|g| g.declaring == a.name) else { continue };
        for m in &own.methods {
            let pm = &methods[&(a.name.clone(), m.name.clone())];
            if !pm.supported() {
                continue;
            }
            gen_c_invoke_case(&mut c, &a.name, pm);
        }
    }
    c.push_str("    }\n    FIXME(\"unsupported method %u\\n\", method);\n    return E_NOTIMPL;\n}\n");

    std::fs::write(format!("{c_dir}/proxy_gen.h"), h).unwrap();
    std::fs::write(format!("{c_dir}/proxy_gen.c"), c).unwrap();

    // ======================= Rust =======================
    let mut r = String::new();
    writeln!(r, "//! Wygenerowane przez tools/gen-stubs (proxy) — NIE EDYTOWAĆ ręcznie.").unwrap();
    writeln!(r, "#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, dead_code, unused_variables, unused_mut, clippy::all)]\n").unwrap();
    writeln!(r, "use crate::rpc::{{Ctx, Dispatch, ObjRef, AppRef, WvResult, S_OK, E_NOTIMPL}};").unwrap();
    writeln!(r, "use crate::protocol::{{Reader, Writer}};\nuse std::any::Any;\nuse std::rc::Rc;\n").unwrap();
    writeln!(r, "pub const API_HASH: u64 = 0x{api_hash:016x};\npub const FAMILY_COUNT: u32 = {};\n", leaves.len()).unwrap();
    for (n, l) in leaves.iter().enumerate() {
        writeln!(r, "pub const F_{}: u32 = {n};", l.name).unwrap();
    }
    for key in &order {
        let m = &methods[key];
        writeln!(r, "pub const M_{}: u32 = {};", m.ident(), m.id).unwrap();
    }
    for (name, _, fields) in STRUCTS {
        writeln!(r, "\n#[repr(C)]\n#[derive(Clone, Copy, Default, Debug, PartialEq)]\npub struct {name} {{").unwrap();
        for (f, t) in fields.iter() {
            writeln!(r, "    pub {f}: {t},").unwrap();
        }
        writeln!(r, "}}").unwrap();
    }
    writeln!(r, "\npub fn family_name(f: u32) -> &'static str {{\n    match f {{").unwrap();
    for (n, l) in leaves.iter().enumerate() {
        writeln!(r, "        {n} => \"{}\",", l.name).unwrap();
    }
    writeln!(r, "        _ => \"?\",\n    }}\n}}").unwrap();

    for l in leaves {
        let leaf = &l.name;
        let fam = family_index[leaf.as_str()];
        writeln!(r, "\n// ===== {leaf} =====\n\npub trait {leaf}Impl: 'static {{").unwrap();
        for key in chain(l) {
            let pm = &methods[&key];
            if !pm.supported() {
                continue;
            }
            gen_rust_trait_method(&mut r, pm, &family_of, &family_index);
        }
        writeln!(r, "}}\n\npub struct {leaf}Obj<T: {leaf}Impl>(pub T);\n").unwrap();
        writeln!(r, "impl<T: {leaf}Impl> Dispatch for {leaf}Obj<T> {{").unwrap();
        writeln!(r, "    fn family(&self) -> u32 {{ {fam} }}\n    fn as_any(&self) -> &dyn Any {{ self }}").unwrap();
        writeln!(r, "    fn dispatch(&self, ctx: &mut Ctx, method: u32, rd: &mut Reader, wr: &mut Writer) -> i32 {{\n        match method {{").unwrap();
        for key in chain(l) {
            let pm = &methods[&key];
            if !pm.supported() {
                continue;
            }
            gen_rust_dispatch_arm(&mut r, pm);
        }
        writeln!(r, "            _ => E_NOTIMPL,\n        }}\n    }}\n}}").unwrap();
        writeln!(r, "\npub struct {leaf}Stub;\nimpl {leaf}Impl for {leaf}Stub {{}}").unwrap();
    }
    writeln!(r, "\n/// Obiekt-zaślepka rodziny (domyślne implementacje: log + S_OK).\npub fn make_stub(family: u32) -> Option<Rc<dyn Dispatch>> {{\n    Some(match family {{").unwrap();
    for (n, l) in leaves.iter().enumerate() {
        writeln!(r, "        {n} => Rc::new({0}Obj({0}Stub)),", l.name).unwrap();
    }
    writeln!(r, "        _ => return None,\n    }})\n}}").unwrap();

    // Klienci obiektów aplikacji.
    for a in app_ifaces {
        let Some(own) = a.groups.iter().find(|g| g.declaring == a.name) else { continue };
        writeln!(r, "\n/// Obiekt aplikacji `{0}` (wywołania przez INVOKE).\n#[derive(Clone)]\npub struct {0}App(pub AppRef);\n\nimpl {0}App {{", a.name).unwrap();
        for m in &own.methods {
            let pm = &methods[&(a.name.clone(), m.name.clone())];
            if !pm.supported() {
                continue;
            }
            gen_rust_app_method(&mut r, pm);
        }
        writeln!(r, "}}").unwrap();
    }
    std::fs::write(rust_file, r).unwrap();
}

// ---------------- C ----------------

fn c_put(c: &mut String, pk: &Pk, name: &str) {
    match pk {
        Pk::I32(_) => writeln!(c, "    wv2_put_u32(&args, (UINT32){name});").unwrap(),
        Pk::I64(_) => writeln!(c, "    wv2_put_u64(&args, (UINT64){name});").unwrap(),
        Pk::F64 => writeln!(c, "    wv2_put_f64(&args, {name});").unwrap(),
        Pk::Str => writeln!(c, "    wv2_put_str(&args, {name});").unwrap(),
        Pk::Struct(s) => writeln!(c, "    wv2_put_bytes(&args, &{name}, sizeof({s}));").unwrap(),
        Pk::Handle => writeln!(c, "    wv2_put_u64(&args, (UINT64)(ULONG_PTR){name});").unwrap(),
        Pk::Iface(_) => writeln!(c, "    wv2_put_iface(&args, (IUnknown *){name});").unwrap(),
        Pk::Stream => writeln!(c, "    wv2_put_stream(&args, {name});").unwrap(),
        Pk::Unsupported(_) => unreachable!(),
    }
}

fn c_get_out(c: &mut String, pk: &Pk, name: &str, ty: &str) {
    let base = ty.replace("const ", "");
    let base = base.trim().strip_suffix('*').unwrap().trim().to_string();
    match pk {
        Pk::I32(_) => writeln!(c, "        *{name} = ({base})wv2_get_u32(&ret);").unwrap(),
        Pk::I64(_) => writeln!(c, "        *{name} = ({base})wv2_get_u64(&ret);").unwrap(),
        Pk::F64 => writeln!(c, "        *{name} = wv2_get_f64(&ret);").unwrap(),
        Pk::Str => writeln!(c, "        *{name} = wv2_get_str_co(&ret);").unwrap(),
        Pk::Struct(s) => writeln!(c, "        wv2_get_bytes(&ret, {name}, sizeof({s}));").unwrap(),
        Pk::Handle => writeln!(c, "        *{name} = ({base})(ULONG_PTR)wv2_get_u64(&ret);").unwrap(),
        Pk::Iface(_) => writeln!(c, "        *{name} = ({base})wv2_get_iface(&ret);").unwrap(),
        Pk::Stream => writeln!(c, "        *{name} = wv2_get_stream(&ret);").unwrap(),
        Pk::Unsupported(_) => unreachable!(),
    }
}

fn gen_c_proxy(c: &mut String, leaf: &str, fname: &str, pm: &PMethod) {
    let m = pm.m;
    let mut sig = format!("{leaf} *iface");
    for p in &m.params {
        let sep = if p.ty.ends_with('*') { "" } else { " " };
        write!(sig, ", {}{sep}{}", p.ty, p.name).unwrap();
    }
    write!(c, "\nstatic {} STDMETHODCALLTYPE {fname}({sig})\n{{\n", m.ret).unwrap();
    if !pm.supported() {
        let why: Vec<String> = pm.kinds.iter().filter_map(|(k, _)| match k { Pk::Unsupported(t) => Some(t.clone()), _ => None }).collect();
        writeln!(c, "    FIXME(\"iface %p: {} not marshallable ({}), stub!\\n\", iface);", pm.ident(), why.join(", ")).unwrap();
        let ret = if m.ret == "HRESULT" { "E_NOTIMPL" } else { "0" };
        writeln!(c, "    return {ret};\n}}").unwrap();
        return;
    }
    writeln!(c, "    struct wv2_buffer args = {{0}};\n    struct wv2_reader ret = {{0}};\n    HRESULT hr;\n").unwrap();
    // Token w add_X jest opcjonalny w WebView2 (WebView2APISample podaje NULL); reszta: E_POINTER.
    let optional = |p: &crate::Param| p.ty.replace(' ', "") == "EventRegistrationToken*";
    for (p, (_, out)) in m.params.iter().zip(&pm.kinds) {
        if *out && !optional(p) {
            writeln!(c, "    if (!{}) return E_POINTER;", p.name).unwrap();
        }
    }
    for (p, (pk, out)) in m.params.iter().zip(&pm.kinds) {
        if !*out {
            c_put(c, pk, &p.name);
        }
    }
    writeln!(c, "    hr = proxy_call((IUnknown *)iface, METHOD_{}, &args, &ret);", pm.ident()).unwrap();
    let outs: Vec<_> = m.params.iter().zip(&pm.kinds).filter(|(_, (_, o))| *o).collect();
    if !outs.is_empty() {
        writeln!(c, "    if (SUCCEEDED(hr))\n    {{").unwrap();
        for (p, (pk, _)) in outs {
            if optional(p) {
                writeln!(c, "        if ({0}) wv2_get_bytes(&ret, {0}, sizeof(*{0}));", p.name).unwrap();
            } else {
                c_get_out(c, pk, &p.name, &p.ty);
            }
        }
        writeln!(c, "    }}").unwrap();
    }
    writeln!(c, "    wv2_reader_free(&ret);\n    return hr;\n}}").unwrap();
}

fn gen_c_invoke_case(c: &mut String, iface: &str, pm: &PMethod) {
    let m = pm.m;
    writeln!(c, "    case METHOD_{}:\n    {{", pm.ident()).unwrap();
    let mut call_args = Vec::new();
    let mut cleanup = String::new();
    for (p, (pk, out)) in m.params.iter().zip(&pm.kinds) {
        let n = format!("a_{}", p.name);
        if *out {
            let base = p.ty.replace("const ", "");
            let base = base.trim().strip_suffix('*').unwrap().trim().to_string();
            match pk {
                Pk::Struct(s) => writeln!(c, "        {s} {n};\n        memset(&{n}, 0, sizeof({n}));").unwrap(),
                _ => writeln!(c, "        {base} {n} = 0;").unwrap(),
            }
            call_args.push(format!("&{n}"));
            continue;
        }
        match pk {
            Pk::I32(_) => writeln!(c, "        {} {n} = ({})wv2_get_u32(args);", p.ty, p.ty).unwrap(),
            Pk::I64(_) => writeln!(c, "        {} {n} = ({})wv2_get_u64(args);", p.ty, p.ty).unwrap(),
            Pk::F64 => writeln!(c, "        double {n} = wv2_get_f64(args);").unwrap(),
            Pk::Str => {
                writeln!(c, "        WCHAR *{n} = wv2_get_str(args);").unwrap();
                writeln!(cleanup, "        free({n});").unwrap();
            }
            Pk::Struct(s) => writeln!(c, "        {s} {n};\n        wv2_get_bytes(args, &{n}, sizeof({n}));").unwrap(),
            Pk::Handle => writeln!(c, "        HWND {n} = (HWND)(ULONG_PTR)wv2_get_u64(args);").unwrap(),
            Pk::Iface(_) => {
                writeln!(c, "        IUnknown *{n} = wv2_get_iface(args);").unwrap();
                writeln!(cleanup, "        if ({n}) IUnknown_Release({n});").unwrap();
            }
            Pk::Stream => {
                writeln!(c, "        IStream *{n} = wv2_get_stream(args);").unwrap();
                writeln!(cleanup, "        if ({n}) IStream_Release({n});").unwrap();
            }
            Pk::Unsupported(_) => unreachable!(),
        }
        call_args.push(match pk {
            Pk::Iface(_) => format!("(void *){n}"),
            _ => n.clone(),
        });
    }
    let args = if call_args.is_empty() { String::new() } else { format!(", {}", call_args.join(", ")) };
    writeln!(c, "        hr = (({iface} *)obj)->lpVtbl->{}(({iface} *)obj{args});", m.name).unwrap();
    for (p, (pk, out)) in m.params.iter().zip(&pm.kinds) {
        if !*out {
            continue;
        }
        let n = format!("a_{}", p.name);
        match pk {
            Pk::I32(_) => writeln!(c, "        wv2_put_u32(out, (UINT32){n});").unwrap(),
            Pk::I64(_) => writeln!(c, "        wv2_put_u64(out, (UINT64){n});").unwrap(),
            Pk::F64 => writeln!(c, "        wv2_put_f64(out, {n});").unwrap(),
            Pk::Str => writeln!(c, "        wv2_put_str(out, {n});\n        CoTaskMemFree({n});").unwrap(),
            Pk::Struct(s) => writeln!(c, "        wv2_put_bytes(out, &{n}, sizeof({s}));").unwrap(),
            Pk::Handle => writeln!(c, "        wv2_put_u64(out, (UINT64)(ULONG_PTR){n});").unwrap(),
            Pk::Iface(_) => writeln!(c, "        wv2_put_iface(out, (IUnknown *){n});\n        if ({n}) IUnknown_Release((IUnknown *){n});").unwrap(),
            Pk::Stream => writeln!(c, "        wv2_put_stream(out, {n});\n        if ({n}) IStream_Release({n});").unwrap(),
            Pk::Unsupported(_) => unreachable!(),
        }
    }
    c.push_str(&cleanup);
    writeln!(c, "        return hr;\n    }}").unwrap();
}

// ---------------- Rust ----------------

fn rust_read(pk: &Pk) -> String {
    match pk {
        Pk::I32(_) | Pk::I64(_) | Pk::F64 | Pk::Handle => match rust_scalar(pk) {
            "bool" => "rd.u32().unwrap_or(0) != 0".into(),
            "u32" => "rd.u32().unwrap_or(0)".into(),
            "i32" => "rd.u32().unwrap_or(0) as i32".into(),
            "u64" => "rd.u64().unwrap_or(0)".into(),
            "i64" => "rd.u64().unwrap_or(0) as i64".into(),
            "f64" => "f64::from_bits(rd.u64().unwrap_or(0))".into(),
            _ => unreachable!(),
        },
        Pk::Str => "rd.str_opt()".into(),
        Pk::Struct(s) => format!("rd.pod::<{s}>()"),
        Pk::Iface(_) => "ctx.read_objref(rd)".into(),
        Pk::Stream => "rd.stream()".into(),
        Pk::Unsupported(_) => unreachable!(),
    }
}

fn rust_write(pk: &Pk, v: &str, w: &str) -> String {
    match pk {
        Pk::I32(_) | Pk::I64(_) | Pk::F64 | Pk::Handle => match rust_scalar(pk) {
            "bool" => format!("{w}.put_u32({v} as u32);"),
            "u32" => format!("{w}.put_u32({v});"),
            "i32" => format!("{w}.put_u32({v} as u32);"),
            "u64" => format!("{w}.put_u64({v});"),
            "i64" => format!("{w}.put_u64({v} as u64);"),
            "f64" => format!("{w}.put_u64({v}.to_bits());"),
            _ => unreachable!(),
        },
        Pk::Str => format!("{w}.put_str(Some(&{v}));"),
        Pk::Struct(_) => format!("{w}.put_pod(&{v});"),
        Pk::Iface(_) => format!("ctx.write_objref({w}, &{v});"),
        Pk::Stream => format!("{w}.put_stream({v}.as_deref());"),
        Pk::Unsupported(_) => unreachable!(),
    }
}

/// Typ zwracany przez metodę traitu: (), T albo krotka.
fn rust_outs<'a>(pm: &'a PMethod) -> (String, Vec<&'a Pk>) {
    let outs: Vec<&Pk> = pm.kinds.iter().filter(|(_, o)| *o).map(|(k, _)| k).collect();
    let ty = match outs.len() {
        0 => "()".to_string(),
        1 => rust_type(outs[0], true),
        _ => format!("({})", outs.iter().map(|k| rust_type(k, true)).collect::<Vec<_>>().join(", ")),
    };
    (ty, outs)
}

fn gen_rust_trait_method(r: &mut String, pm: &PMethod, family_of: &HashMap<&str, &str>, family_index: &HashMap<&str, usize>) {
    let mut params = String::new();
    for (p, (pk, out)) in pm.m.params.iter().zip(&pm.kinds) {
        if !*out {
            write!(params, ", {}: {}", rust_ident(&p.name), rust_type(pk, false)).unwrap();
        }
    }
    let (ty, outs) = rust_outs(pm);
    let defaults: Vec<String> = outs.iter().map(|k| match k {
        Pk::Iface(i) => match family_of.get(i.as_str()) {
            Some(f) => format!("ctx.stub({})", family_index[f]),
            None => "ObjRef::Null".into(),
        },
        Pk::Str => "String::new()".into(),
        Pk::Stream => "None".into(),
        _ => "Default::default()".into(),
    }).collect();
    let value = match defaults.len() {
        0 => "()".to_string(),
        1 => defaults[0].clone(),
        _ => format!("({})", defaults.join(", ")),
    };
    writeln!(r, "    fn {}(&self, ctx: &mut Ctx{params}) -> WvResult<{ty}> {{\n        ctx.fixme(\"{}::{}\");\n        Ok({value})\n    }}",
             pm.m.name, pm.decl, pm.m.name).unwrap();
}

fn gen_rust_dispatch_arm(r: &mut String, pm: &PMethod) {
    writeln!(r, "            M_{} => {{", pm.ident()).unwrap();
    let mut args = Vec::new();
    for (p, (pk, out)) in pm.m.params.iter().zip(&pm.kinds) {
        if !*out {
            let n = format!("a_{}", p.name);
            writeln!(r, "                let {n} = {};", rust_read(pk)).unwrap();
            args.push(n);
        }
    }
    let call = format!("self.0.{}(ctx{})", pm.m.name, args.iter().map(|a| format!(", {a}")).collect::<String>());
    let (_, outs) = rust_outs(pm);
    match outs.len() {
        0 => writeln!(r, "                match {call} {{ Ok(()) => S_OK, Err(e) => e }}").unwrap(),
        1 => writeln!(r, "                match {call} {{ Ok(v) => {{ {} S_OK }} Err(e) => e }}", rust_write(outs[0], "v", "wr")).unwrap(),
        _ => {
            let names: Vec<String> = (0..outs.len()).map(|i| format!("v{i}")).collect();
            let writes: String = outs.iter().zip(&names).map(|(k, n)| rust_write(k, n, "wr") + " ").collect();
            writeln!(r, "                match {call} {{ Ok(({})) => {{ {writes}S_OK }} Err(e) => e }}", names.join(", ")).unwrap();
        }
    }
    writeln!(r, "            }}").unwrap();
}

fn gen_rust_app_method(r: &mut String, pm: &PMethod) {
    let mut params = String::new();
    let mut writes = String::new();
    for (p, (pk, out)) in pm.m.params.iter().zip(&pm.kinds) {
        if *out {
            continue;
        }
        let n = rust_ident(&p.name);
        let ty = match pk {
            Pk::Iface(_) => "&ObjRef".to_string(),
            Pk::Str => "Option<&str>".to_string(),
            Pk::Stream => "Option<&[u8]>".to_string(),
            other => rust_type(other, false),
        };
        write!(params, ", {n}: {ty}").unwrap();
        let w = match pk {
            Pk::Iface(_) => format!("ctx.write_objref(&mut w, {n});"),
            Pk::Str => format!("w.put_str({n});"),
            Pk::Stream => format!("w.put_stream({n});"),
            other => rust_write(other, &n, "w"),
        };
        writeln!(writes, "        {w}").unwrap();
    }
    let (ty, outs) = rust_outs(pm);
    let reads: Vec<String> = outs.iter().map(|k| match k {
        Pk::Str => "rd.str_opt().unwrap_or_default()".to_string(),
        _ => rust_read(k),
    }).collect();
    let value = match reads.len() {
        0 => "()".to_string(),
        1 => reads[0].clone(),
        _ => format!("({})", reads.join(", ")),
    };
    writeln!(r, "    /// Synchroniczne (czeka na wynik, obsługując w tym czasie wywołania z DLL).").unwrap();
    writeln!(r, "    pub fn {}(&self, ctx: &mut Ctx{params}) -> WvResult<{ty}> {{\n        let mut w = Writer::default();\n{writes}        let reply = ctx.invoke(&self.0, M_{}, w)?;\n        let rd = &mut Reader(&reply);\n        Ok({value})\n    }}",
             pm.m.name, pm.ident()).unwrap();
    writeln!(r, "    /// Asynchroniczne (bez czekania; kolejność zachowana).").unwrap();
    writeln!(r, "    pub fn {}_async(&self, ctx: &mut Ctx{params}) {{\n        let mut w = Writer::default();\n{writes}        ctx.invoke_async(&self.0, M_{}, w);\n    }}",
             pm.m.name, pm.ident()).unwrap();
}

#[allow(dead_code)]
fn unused() {
    let _ = die;
}
