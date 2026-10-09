#include <gtk/gtk.h>
#include "../Shared/DateOptions.hpp"

namespace {
struct State {
  GtkWindow *parent=nullptr;
  GtkWidget *dialog=nullptr;
  GtkCalendar *calendar=nullptr;
  std::shared_ptr<dotnative::Reply> reply;
  std::unique_ptr<dotnative_dates::Options> options;
  bool adjusting=false;
};
using StatePtr = std::shared_ptr<State>;
dotnative_dates::Date Selected(GtkCalendar *calendar) {
  GDateTime *time=gtk_calendar_get_date(calendar);
  dotnative_dates::Date date{g_date_time_get_year(time),g_date_time_get_month(time),g_date_time_get_day_of_month(time)};
  g_date_time_unref(time); return date;
}
void Select(GtkCalendar *calendar, dotnative_dates::Date date) {
  auto *time=g_date_time_new_local(date.y,date.m,date.d,12,0,0);
  gtk_calendar_select_day(calendar,time); g_date_time_unref(time);
}
void Finish(const StatePtr &state, bool confirm) {
  if(!state->dialog) return;
  auto *dialog=state->dialog; state->dialog=nullptr;
  auto reply=std::exchange(state->reply,nullptr);
  if(reply) {
    if(confirm) reply->Success(dotnative_dates::Format(Selected(state->calendar)));
    else reply->Success();
  }
  state->calendar=nullptr; state->options.reset(); gtk_window_destroy(GTK_WINDOW(dialog));
}
void Response(GtkDialog *, int response, gpointer data) { Finish(*static_cast<StatePtr*>(data),response==GTK_RESPONSE_OK); }
void DayChanged(GtkCalendar *calendar, gpointer data) {
  auto state=*static_cast<StatePtr*>(data);
  if(state->adjusting || !state->options) return;
  auto date=Selected(calendar);
  if(date < state->options->minimum || date > state->options->maximum) {
    state->adjusting=true;
    Select(calendar, date < state->options->minimum ? state->options->minimum : state->options->maximum);
    state->adjusting=false;
  }
}
void FreeState(gpointer data, GClosure *) { delete static_cast<StatePtr*>(data); }
}
void DotNativeNativeDatePickerPlugin(const dotnative::PluginRegistrar &registrar) {
  auto state=std::make_shared<State>(); state->parent=static_cast<GtkWindow*>(registrar.NativeWindow());
  auto &channel=registrar.Channel("dotnative.nativedatepicker");
  channel.OnReset=[state]{ Finish(state,false); };
  channel.Handle("cancel",[state](const auto&,auto reply){Finish(state,false);reply->Success();});
  channel.Handle("show",[state](const dotnative::Value &args,auto reply){
    if(state->dialog || !state->parent){ reply->Error("picker_unavailable","No active window, or a picker is already open"); return; }
    try { state->options=std::make_unique<dotnative_dates::Options>(args); }
    catch(const std::exception &error){reply->Error("invalid_arguments",error.what());return;}
    auto *dialog=gtk_dialog_new(); state->dialog=dialog; state->reply=reply;
    gtk_window_set_title(GTK_WINDOW(dialog),state->options->title.c_str());
    gtk_window_set_transient_for(GTK_WINDOW(dialog),state->parent);gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dialog),state->options->cancel.c_str(),GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(dialog),state->options->confirm.c_str(),GTK_RESPONSE_OK);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog),GTK_RESPONSE_OK);
    auto *calendar=gtk_calendar_new();state->calendar=GTK_CALENDAR(calendar);
    Select(state->calendar,state->options->initial);
    gtk_widget_set_margin_top(calendar,16);gtk_widget_set_margin_bottom(calendar,16);gtk_widget_set_margin_start(calendar,16);gtk_widget_set_margin_end(calendar,16);
    gtk_box_append(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),calendar);
    g_signal_connect_data(dialog,"response",G_CALLBACK(Response),new StatePtr(state),FreeState,G_CONNECT_DEFAULT);
    g_signal_connect_data(calendar,"day-selected",G_CALLBACK(DayChanged),new StatePtr(state),FreeState,G_CONNECT_DEFAULT);
    reply->OnCancel=[state]{Finish(state,false);};
    gtk_window_present(GTK_WINDOW(dialog));
  });
}
