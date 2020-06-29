// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtWidgets>
#include <QWebChannel>

#include "wallet/view_iface.h"

#ifndef Q_MOC_RUN
#include "wallet/wallets_manager.h"
#include "currency_core/offers_services_helpers.h"
#endif

QT_BEGIN_NAMESPACE
class QWebEngineView;
class QLineEdit;
QT_END_NAMESPACE


#define  APP_DATA_FILE_BINARY_SIGNATURE   0x1000111101101021LL


// class MediatorObject : public QObject
// {
//   Q_OBJECT
// 
// public:
// 
// signals :
//   /*!
//   This signal is emitted from the C++ side and the text displayed on the HTML client side.
//   */
//   void from_c_to_html(const QString &text);
// 
//   public slots:
//         /*!
//         This slot is invoked from the HTML client side and the text displayed on the server side.
//         */
//   void from_html_to_c(const QString &text);
// };

//
class MainWindow : public QMainWindow, 
                   public currency::i_core_event_handler,
                   public view::i_view, 
                   public QAbstractNativeEventFilter
{
  Q_OBJECT

public:
  MainWindow();
  ~MainWindow();

  bool init_backend(int argc, char* argv[]);
  bool show_inital();
  void show_notification(const std::string& title, const std::string& message);

  struct app_config
  {
    std::pair<int64_t, int64_t> m_window_position;
    std::pair<int64_t, int64_t> m_window_size;
    bool is_maximazed;
    bool is_showed;
  };

  protected slots:

  void on_load_finished(bool ok);
  bool    do_close();


  public slots:
  QString show_openfile_dialog(const QString& param);
  QString show_savefile_dialog(const QString& param);
  QString open_wallet(const QString& param);
  QString get_my_offers(const QString& param);
  QString get_fav_offers(const QString& param);
  QString generate_wallet(const QString& param);
  QString run_wallet(const QString& param);
  QString close_wallet(const QString& wallet_id);
  QString get_contracts(const QString& wallet_id);
  QString create_proposal(const QString& param);
  QString accept_proposal(const QString& param);
  QString release_contract(const QString& param);
  QString request_cancel_contract(const QString& param);
  QString accept_cancel_contract(const QString& param);


  QString get_version();
  QString get_os_version();
  QString transfer(const QString& json_transfer_object);
  QString have_secure_app_data();
  QString drop_secure_app_data();
  QString get_secure_app_data(const QString& param);
  QString store_secure_app_data(const QString& param);
  QString set_master_password(const QString& param);
  QString check_master_password(const QString& param);
  QString get_app_data();
  QString store_app_data(const QString& param);
  QString get_default_user_dir(const QString& param);
//  QString get_all_offers(const QString& param);
  QString get_offers_ex(const QString& param);
  QString push_offer(const QString& param);
  QString cancel_offer(const QString& param);
  QString push_update_offer(const QString& param);
  QString get_alias_info_by_address(const QString& param);
  QString get_alias_info_by_name(const QString& param);
  QString get_all_aliases();
  QString request_alias_registration(const QString& param);
  QString request_alias_update(const QString& param);
  QString get_alias_coast(const QString& param);
  QString validate_address(const QString& param);
  QString on_request_quit();
  QString resync_wallet(const QString& param);
  QString get_recent_transfers(const QString& param);
  QString get_mining_history(const QString& param);
  QString start_pos_mining(const QString& param);
  QString stop_pos_mining(const QString& param);
  QString set_log_level(const QString& param);
  QString get_log_level(const QString& param);
//  QString dump_all_offers();
  QString webkit_launched_script();
  QString get_smart_wallet_info(const QString& param);
  QString restore_wallet(const QString& param);
  QString is_pos_allowed();
  QString store_to_file(const QString& path, const QString& buff);
  QString load_from_file(const QString& path);
  QString is_file_exist(const QString& path);
  QString get_mining_estimate(const QString& obj);
  QString backup_wallet_keys(const QString& obj);
  QString reset_wallet_password(const QString& param);
  QString is_wallet_password_valid(const QString& param);
  QString is_autostart_enabled();
  QString toggle_autostart(const QString& param);
  QString is_valid_restore_wallet_text(const QString& param);
  QString print_text(const QString& param);
  QString print_log(const QString& param);
  QString set_clipboard(const QString& param);
  QString set_localization_strings(const QString str);
  QString get_clipboard();
  void    message_box(const QString& msg);
  bool    toggle_mining();
  QString get_exchange_last_top(const QString& params);
  QString get_tx_pool_info();
  QString get_default_fee();
  QString get_options();  
  void    bool_toggle_icon(const QString& param);
  QString get_log_file();
  QString check_available_sources(const QString& param);
  QString open_url_in_browser(const QString& param);

  void    trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void    tray_quit_requested();
  void    on_menu_show();
  QString is_remnotenode_mode_preconfigured();
  QString start_backend(const QString& params);

signals:
  void quit_requested(const QString str);
  void update_daemon_state(const QString str);
  void update_wallet_status(const QString str);
  void update_wallet_info(const QString str);
  void money_transfer(const QString str);
  void money_transfer_cancel(const QString str);
  void wallet_sync_progress(const QString str);
  void handle_internal_callback(const QString str, const QString callback_name);
  void update_pos_mining_text(const QString str);
  void do_dispatch(const QString status, const QString params);  //general function
  void on_core_event(const QString method_name);  //general function
  void set_options(const QString str);  //general function

private:
  //--------------------  i_core_event_handler --------------------
  virtual void on_core_event(const std::string event_name, const currency::core_event_v& e);
  virtual void on_complete_events();
  virtual void on_clear_events();

  //------- i_view ---------
  virtual bool update_daemon_status(const view::daemon_status_info& info);
  virtual bool on_backend_stopped();
  virtual bool show_msg_box(const std::string& message);
  virtual bool update_wallet_status(const view::wallet_status_info& wsi);
  virtual bool update_wallets_info(const view::wallets_summary_info& wsi);
  virtual bool money_transfer(const view::transfer_event_info& tei);
  virtual bool wallet_sync_progress(const view::wallet_sync_progres_param& p);
  virtual bool money_transfer_cancel(const view::transfer_event_info& wsi);
  virtual bool init(const std::string& path);
  virtual bool pos_block_found(const currency::block& block_found);
  virtual bool set_options(const view::gui_options& opt);
  //--------- QAbstractNativeEventFilter ---------------------------
  virtual bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
  //----------------------------------------------


  void closeEvent(QCloseEvent *event);
  void contextMenuEvent(QContextMenuEvent * event);
  void changeEvent(QEvent *e);
  void on_maximized();

  //void setOrientation(Qt::ScreenOrientation orientation);
  
  

  void init_tray_icon(const std::string& htmlPath);
  bool set_html_path(const std::string& path);
  void load_file(const QString &fileName);
  void store_pos(bool consider_showed = false);
  void store_window_pos();
  void restore_pos(bool consider_showed = false);
  bool store_app_config();
  bool load_app_config();
  bool init_window();

  std::string get_wallet_log_prefix(size_t wallet_id) const { return m_backend.get_wallet_log_prefix(wallet_id); }


  //MediatorObject mo;
  // UI
  QWebEngineView *m_view;
  QWebChannel* m_channel;

  // DATA
  wallets_manager m_backend;
  //std::atomic<bool> m_quit_requested;
  std::atomic<bool> m_gui_deinitialize_done_1;
  std::atomic<bool> m_backend_stopped_2;
  std::atomic<bool> m_system_shutdown;

  std::string m_master_password;


  app_config m_config;

  epee::locked_object<std::map<uint64_t, uint64_t>> m_wallet_states;
  struct events_que_struct
  {
    std::list<currency::core_event> m_que;
    
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(m_que)
    END_KV_SERIALIZE_MAP()
  };
  events_que_struct m_events;

  std::vector<std::string> m_localization;

  enum localization_string_indices
  {
    // order is surprizingly important here! (see also updateLocalisation in AppController.js)
    localization_id_quit = 0, 
    localization_id_is_received,
    localization_id_is_confirmed, 
    localization_id_income_transfer_unconfirmed, 
    localization_id_income_transfer_confirmed,
    localization_id_mined,
    localization_id_locked,
    localization_id_minimized_text,
    localization_id_minimized_title,
    localization_id_tray_menu_show,
    localization_id_tray_menu_minimize,

    localization_id_couter // keep it at the end of list
  };

  std::string m_normal_icon_path;
  std::string m_blocked_icon_path;

  std::unique_ptr<QSystemTrayIcon> m_tray_icon;
  std::unique_ptr<QMenu>   m_tray_icon_menu;
  std::unique_ptr<QAction> m_restore_action;
  std::unique_ptr<QAction> m_quit_action;
  std::unique_ptr<QAction> m_minimize_action;

  std::string m_last_update_daemon_status_json;

  template<typename argument_type, typename response_type, typename callback_t>
  QString prepare_call(const char* name, const QString& param, callback_t cb)
  {
    LOG_PRINT_L0("que_call: [" << name << "]");
    view::api_response_t<view::api_void> ar;
    argument_type wio = AUTO_VAL_INIT(wio);
    if (!epee::serialization::load_t_from_json(wio, param.toStdString()))
    {
      ar.error_code = API_RETURN_CODE_BAD_ARG;
      return epee::serialization::store_t_to_json(ar).c_str();
    }
    cb(wio, ar);
    return epee::serialization::store_t_to_json(ar).c_str();
  }
};

namespace boost
{
  namespace serialization
  {
    template<class archive_t>
    void serialize(archive_t & ar, MainWindow::app_config& ac, const unsigned int version)
    {
      ar & ac.is_maximazed;
      ar & ac.is_showed;
      ar & ac.m_window_position;
      ar & ac.m_window_size;
    }
  }
}
