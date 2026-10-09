#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include "../Shared/DateOptions.hpp"
#pragma comment(lib,"comctl32.lib")
namespace {
struct State { HWND parent=nullptr, dialog=nullptr, picker=nullptr; std::shared_ptr<dotnative::Reply> reply; };
std::wstring Wide(const std::string &text) {
  int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);
  if(!length&&!text.empty()) throw std::invalid_argument("Invalid UTF-8 label");
  std::wstring result(length,0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),result.data(),length);return result;
}
SYSTEMTIME Time(dotnative_dates::Date date){SYSTEMTIME result{};result.wYear=WORD(date.y);result.wMonth=WORD(date.m);result.wDay=WORD(date.d);return result;}
void Finish(State *state,bool confirm) {
  if(!state->dialog)return;
  auto reply=std::exchange(state->reply,nullptr);
  if(reply){
    SYSTEMTIME time{};
    if(confirm&&DateTime_GetSystemtime(state->picker,&time)==GDT_VALID)
      reply->Success(dotnative_dates::Format({int(time.wYear),int(time.wMonth),int(time.wDay)}));
    else reply->Success();
  }
  auto dialog=state->dialog;state->dialog=nullptr;state->picker=nullptr;
  DestroyWindow(dialog);EnableWindow(state->parent,TRUE);SetForegroundWindow(state->parent);
}
LRESULT CALLBACK Procedure(HWND window,UINT message,WPARAM wparam,LPARAM lparam) {
  auto *state=reinterpret_cast<State*>(GetWindowLongPtrW(window,GWLP_USERDATA));
  if(message==WM_NCCREATE){state=static_cast<State*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(state));}
  if(state&&message==WM_CLOSE){Finish(state,false);return 0;}
  if(state&&message==WM_COMMAND&&(LOWORD(wparam)==IDOK||LOWORD(wparam)==IDCANCEL)){Finish(state,LOWORD(wparam)==IDOK);return 0;}
  return DefWindowProcW(window,message,wparam,lparam);
}
LRESULT CALLBACK Keyboard(HWND window,UINT message,WPARAM key,LPARAM lparam,UINT_PTR id,DWORD_PTR) {
  if(message==WM_KEYDOWN&&(key==VK_RETURN||key==VK_ESCAPE)){PostMessageW(GetParent(window),WM_COMMAND,key==VK_RETURN?IDOK:IDCANCEL,0);return 0;}
  if(message==WM_KEYDOWN&&key==VK_TAB){SetFocus(GetNextDlgTabItem(GetParent(window),window,GetKeyState(VK_SHIFT)<0));return 0;}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(window,Keyboard,id);
  return DefSubclassProc(window,message,key,lparam);
}
}
void DotNativeNativeDatePickerPlugin(const dotnative::PluginRegistrar &registrar) {
  auto state=std::make_shared<State>();state->parent=static_cast<HWND>(registrar.NativeWindow());
  auto &channel=registrar.Channel("dotnative.nativedatepicker");
  channel.OnReset=[state]{Finish(state.get(),false);};
  channel.Handle("cancel",[state](const auto&,auto reply){Finish(state.get(),false);reply->Success();});
  channel.Handle("show",[state](const dotnative::Value &args,auto reply){
    if(state->dialog||!state->parent||!IsWindow(state->parent)){reply->Error("picker_unavailable","No active window, or a picker is already open");return;}
    try {
      dotnative_dates::Options options(args);
      INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_DATE_CLASSES};InitCommonControlsEx(&controls);
      WNDCLASSW type{};type.hInstance=GetModuleHandleW(nullptr);type.lpfnWndProc=Procedure;type.lpszClassName=L"DotNativeNativeDateDialog";type.hCursor=LoadCursorW(nullptr,IDC_ARROW);type.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
      RegisterClassW(&type);
      RECT parent{};GetWindowRect(state->parent,&parent);
      float scale=GetDpiForWindow(state->parent)/96.f;
      auto px=[scale](int value){return int(value*scale);};
      auto title=Wide(options.title),confirm=Wide(options.confirm),cancel=Wide(options.cancel);
      state->dialog=CreateWindowExW(WS_EX_DLGMODALFRAME,type.lpszClassName,title.c_str(),WS_CAPTION|WS_SYSMENU,
        parent.left+(parent.right-parent.left-px(360))/2,parent.top+(parent.bottom-parent.top-px(180))/2,px(360),px(180),state->parent,nullptr,type.hInstance,state.get());
      if(!state->dialog)throw std::runtime_error("Could not create the date dialog");
      state->reply=reply;
      state->picker=CreateWindowExW(0,DATETIMEPICK_CLASSW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|DTS_SHORTDATEFORMAT,px(24),px(24),px(296),px(28),state->dialog,nullptr,type.hInstance,nullptr);
      if(!state->picker)throw std::runtime_error("Could not create the date control");
      SYSTEMTIME bounds[]={Time(options.minimum),Time(options.maximum)},initial=Time(options.initial);
      DateTime_SetRange(state->picker,GDTR_MIN|GDTR_MAX,bounds);DateTime_SetSystemtime(state->picker,GDT_VALID,&initial);
      auto cancelButton=CreateWindowExW(0,L"BUTTON",cancel.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,px(124),px(76),px(92),px(32),state->dialog,reinterpret_cast<HMENU>(IDCANCEL),type.hInstance,nullptr);
      auto confirmButton=CreateWindowExW(0,L"BUTTON",confirm.c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,px(224),px(76),px(92),px(32),state->dialog,reinterpret_cast<HMENU>(IDOK),type.hInstance,nullptr);
      for(auto control:{state->picker,cancelButton,confirmButton}){SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);SetWindowSubclass(control,Keyboard,1,0);}
      reply->OnCancel=[state]{Finish(state.get(),false);};EnableWindow(state->parent,FALSE);ShowWindow(state->dialog,SW_SHOW);SetFocus(state->picker);
    }catch(const std::exception &error){ auto old=std::exchange(state->reply,nullptr);Finish(state.get(),false);reply->Error("picker_failed",error.what());}
  });
}
