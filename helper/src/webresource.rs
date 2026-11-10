//! WebResourceRequested po stronie serwera: żądanie, odpowiedź, nagłówki, deferral, argumenty.
//! Żądanie wstrzymuje resources.rs (OnBeforeResourceLoad, CONTINUE_ASYNC); tu, na wątku UI,
//! wołamy handlery aplikacji i po ich decyzji (albo Complete() deferrala) zwalniamy żądanie.

use crate::api_gen::*;
use crate::resources::AppResponse;
use crate::rpc::{Ctx, ObjRef, WvResult, E_INVALIDARG};
use std::cell::{Cell, RefCell};
use std::rc::Rc;

type Headers = Rc<RefCell<Vec<(String, String)>>>;

fn rc<T: crate::rpc::Dispatch + 'static>(obj: T) -> ObjRef {
    ObjRef::Runtime(Rc::new(obj))
}

fn parse_headers(text: &str) -> Vec<(String, String)> {
    text.lines()
        .filter_map(|l| l.split_once(':'))
        .map(|(n, v)| (n.trim().to_string(), v.trim().to_string()))
        .filter(|(n, _)| !n.is_empty())
        .collect()
}

fn find(headers: &Headers, name: &str) -> Option<String> {
    headers.borrow().iter().find(|(n, _)| n.eq_ignore_ascii_case(name)).map(|(_, v)| v.clone())
}

fn not_found() -> i32 {
    0x8007_0490_u32 as i32 // HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
}

// ---------- iterator ----------

pub struct HeaderIterator {
    headers: Headers,
    name: Option<String>,
    pos: Cell<Option<usize>>,
}

impl HeaderIterator {
    fn new(headers: Headers, name: Option<String>) -> ObjRef {
        let it = HeaderIterator { headers, name, pos: Cell::new(None) };
        it.pos.set(it.next_from(0));
        rc(ICoreWebView2HttpHeadersCollectionIteratorObj(it))
    }
    fn next_from(&self, start: usize) -> Option<usize> {
        let h = self.headers.borrow();
        (start..h.len()).find(|&i| self.name.as_ref().is_none_or(|n| h[i].0.eq_ignore_ascii_case(n)))
    }
}

impl ICoreWebView2HttpHeadersCollectionIteratorImpl for HeaderIterator {
    fn GetCurrentHeader(&self, _ctx: &mut Ctx) -> WvResult<(String, String)> {
        let pos = self.pos.get().ok_or(0x8007_0585_u32 as i32)?; // ERROR_INVALID_INDEX
        Ok(self.headers.borrow()[pos].clone())
    }
    fn get_HasCurrentHeader(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.pos.get().is_some())
    }
    fn MoveNext(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        if let Some(p) = self.pos.get() {
            self.pos.set(self.next_from(p + 1));
        }
        Ok(self.pos.get().is_some())
    }
}

// ---------- kolekcje nagłówków ----------

pub struct RequestHeaders(Headers);

impl ICoreWebView2HttpRequestHeadersImpl for RequestHeaders {
    fn GetHeader(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<String> {
        find(&self.0, &name.ok_or(E_INVALIDARG)?).ok_or_else(not_found)
    }
    fn GetHeaders(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<ObjRef> {
        Ok(HeaderIterator::new(self.0.clone(), Some(name.ok_or(E_INVALIDARG)?)))
    }
    fn Contains(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<bool> {
        Ok(find(&self.0, &name.ok_or(E_INVALIDARG)?).is_some())
    }
    fn SetHeader(&self, ctx: &mut Ctx, name: Option<String>, value: Option<String>) -> WvResult<()> {
        ctx.fixme("ICoreWebView2HttpRequestHeaders::SetHeader (not sent to the network)");
        let (name, value) = (name.ok_or(E_INVALIDARG)?, value.ok_or(E_INVALIDARG)?);
        let mut h = self.0.borrow_mut();
        h.retain(|(n, _)| !n.eq_ignore_ascii_case(&name));
        h.push((name, value));
        Ok(())
    }
    fn RemoveHeader(&self, ctx: &mut Ctx, name: Option<String>) -> WvResult<()> {
        ctx.fixme("ICoreWebView2HttpRequestHeaders::RemoveHeader (not applied to the network)");
        let name = name.ok_or(E_INVALIDARG)?;
        self.0.borrow_mut().retain(|(n, _)| !n.eq_ignore_ascii_case(&name));
        Ok(())
    }
    fn GetIterator(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(HeaderIterator::new(self.0.clone(), None))
    }
}

pub struct ResponseHeaders(Headers);

impl ICoreWebView2HttpResponseHeadersImpl for ResponseHeaders {
    fn AppendHeader(&self, _ctx: &mut Ctx, name: Option<String>, value: Option<String>) -> WvResult<()> {
        self.0.borrow_mut().push((name.ok_or(E_INVALIDARG)?, value.ok_or(E_INVALIDARG)?));
        Ok(())
    }
    fn Contains(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<bool> {
        Ok(find(&self.0, &name.ok_or(E_INVALIDARG)?).is_some())
    }
    fn GetHeader(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<String> {
        find(&self.0, &name.ok_or(E_INVALIDARG)?).ok_or_else(not_found)
    }
    fn GetHeaders(&self, _ctx: &mut Ctx, name: Option<String>) -> WvResult<ObjRef> {
        Ok(HeaderIterator::new(self.0.clone(), Some(name.ok_or(E_INVALIDARG)?)))
    }
    fn GetIterator(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(HeaderIterator::new(self.0.clone(), None))
    }
}

// ---------- żądanie i odpowiedź ----------

pub struct Request {
    uri: RefCell<String>,
    method: RefCell<String>,
    headers: Headers,
    content: RefCell<Option<Vec<u8>>>,
}

impl ICoreWebView2WebResourceRequestImpl for Request {
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.borrow().clone())
    }
    fn put_Uri(&self, ctx: &mut Ctx, uri: Option<String>) -> WvResult<()> {
        ctx.fixme("ICoreWebView2WebResourceRequest::put_Uri (redirect not implemented)");
        *self.uri.borrow_mut() = uri.ok_or(E_INVALIDARG)?;
        Ok(())
    }
    fn get_Method(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.method.borrow().clone())
    }
    fn put_Method(&self, ctx: &mut Ctx, method: Option<String>) -> WvResult<()> {
        ctx.fixme("ICoreWebView2WebResourceRequest::put_Method (not applied)");
        *self.method.borrow_mut() = method.ok_or(E_INVALIDARG)?;
        Ok(())
    }
    fn get_Content(&self, _ctx: &mut Ctx) -> WvResult<Option<Vec<u8>>> {
        Ok(self.content.borrow().clone())
    }
    fn put_Content(&self, ctx: &mut Ctx, content: Option<Vec<u8>>) -> WvResult<()> {
        ctx.fixme("ICoreWebView2WebResourceRequest::put_Content (not applied)");
        *self.content.borrow_mut() = content;
        Ok(())
    }
    fn get_Headers(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(rc(ICoreWebView2HttpRequestHeadersObj(RequestHeaders(self.headers.clone()))))
    }
}

pub struct Response {
    content: RefCell<Option<Vec<u8>>>,
    status: Cell<i32>,
    reason: RefCell<String>,
    headers: Headers,
}

pub type ResponseObj = ICoreWebView2WebResourceResponseObj<Response>;

pub fn create_response(content: Option<Vec<u8>>, status: i32, reason: String, headers: Option<&str>) -> ObjRef {
    rc(ICoreWebView2WebResourceResponseObj(Response {
        content: RefCell::new(content),
        status: Cell::new(status),
        reason: RefCell::new(reason),
        headers: Rc::new(RefCell::new(headers.map(parse_headers).unwrap_or_default())),
    }))
}

impl ICoreWebView2WebResourceResponseImpl for Response {
    fn get_Content(&self, _ctx: &mut Ctx) -> WvResult<Option<Vec<u8>>> {
        Ok(self.content.borrow().clone())
    }
    fn put_Content(&self, _ctx: &mut Ctx, content: Option<Vec<u8>>) -> WvResult<()> {
        *self.content.borrow_mut() = content;
        Ok(())
    }
    fn get_Headers(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(rc(ICoreWebView2HttpResponseHeadersObj(ResponseHeaders(self.headers.clone()))))
    }
    fn get_StatusCode(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.status.get())
    }
    fn put_StatusCode(&self, _ctx: &mut Ctx, status: i32) -> WvResult<()> {
        self.status.set(status);
        Ok(())
    }
    fn get_ReasonPhrase(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.reason.borrow().clone())
    }
    fn put_ReasonPhrase(&self, _ctx: &mut Ctx, reason: Option<String>) -> WvResult<()> {
        *self.reason.borrow_mut() = reason.unwrap_or_default();
        Ok(())
    }
}

// ---------- zdarzenie ----------

struct ResourceEvent {
    id: u64,
    context: i32,
    request: ObjRef,
    response: RefCell<ObjRef>,
    finished: Cell<bool>,
    deferred: Cell<bool>,
    dispatching: Cell<bool>,
}

impl ResourceEvent {
    /// Zwalnia wstrzymane żądanie z odpowiedzią aplikacji (albo do sieci, gdy jej brak).
    fn finish(&self) {
        if self.finished.replace(true) {
            return;
        }
        let response = self.response.borrow().clone();
        let app = match &response {
            ObjRef::Null => None,
            r => match r.downcast::<ResponseObj>() {
                Some(obj) => {
                    let r = &obj.0;
                    Some(AppResponse {
                        status: r.status.get(),
                        reason: r.reason.borrow().clone(),
                        headers: r.headers.borrow().clone(),
                        content: r.content.borrow().clone().unwrap_or_default(),
                    })
                }
                None => {
                    eprintln!("helper: fixme response implemented by the application, ignored");
                    None
                }
            },
        };
        crate::resources::finish(self.id, app);
    }
}

pub struct Deferral(Rc<ResourceEvent>);

impl ICoreWebView2DeferralImpl for Deferral {
    fn Complete(&self, _ctx: &mut Ctx) -> WvResult<()> {
        if self.0.dispatching.get() {
            self.0.deferred.set(false); // zakończone jeszcze w handlerze
        } else {
            self.0.finish();
        }
        Ok(())
    }
}

pub struct Args(Rc<ResourceEvent>);

impl ICoreWebView2WebResourceRequestedEventArgs2Impl for Args {
    fn get_Request(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.0.request.clone())
    }
    fn get_Response(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.0.response.borrow().clone())
    }
    fn put_Response(&self, _ctx: &mut Ctx, response: ObjRef) -> WvResult<()> {
        *self.0.response.borrow_mut() = response;
        Ok(())
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        self.0.deferred.set(true);
        Ok(rc(ICoreWebView2DeferralObj(Deferral(self.0.clone()))))
    }
    fn get_ResourceContext(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.0.context)
    }
    fn get_RequestedSourceKind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(1) // COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_DOCUMENT
    }
}

/// Wątek UI: wstrzymane żądanie `id` widoku `view`. Woła handlery i decyduje.
#[allow(clippy::too_many_arguments)]
pub fn handle_request(view: u32, id: u64, context: u32, url: String, method: String, headers: String, body: Vec<u8>) {
    let Some((handlers, sender)) = crate::webview::resource_handlers(view) else {
        crate::resources::finish(id, None);
        return;
    };
    let request = rc(ICoreWebView2WebResourceRequestObj(Request {
        uri: RefCell::new(url),
        method: RefCell::new(method),
        headers: Rc::new(RefCell::new(parse_headers(&headers))),
        content: RefCell::new(if body.is_empty() { None } else { Some(body) }),
    }));
    let event = Rc::new(ResourceEvent {
        id,
        context: context as i32,
        request,
        response: RefCell::new(ObjRef::Null),
        finished: Cell::new(false),
        deferred: Cell::new(false),
        dispatching: Cell::new(true),
    });
    let args = rc(ICoreWebView2WebResourceRequestedEventArgs2Obj(Args(event.clone())));
    for h in handlers {
        if let Err(e) = ICoreWebView2WebResourceRequestedEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args) {
            eprintln!("helper: WebResourceRequested handler failed: {e:#x}");
        }
    }
    event.dispatching.set(false);
    if !event.deferred.get() {
        event.finish();
    }
}
