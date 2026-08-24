// Copyright (c) 2026 Zano Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

class MainWindow;

/**
 * Explicit JavaScript-facing facade for the GUI WebChannel API.
 */
class WebChannelBridge final : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WebChannelBridge)

public:
  explicit WebChannelBridge(MainWindow& main_window, QObject* parent = nullptr);
  ~WebChannelBridge() override = default;

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

  QString on_request_quit(const QString& param);
  QString get_version(const QString& param);
  QString get_os_version(const QString& param);
  QString get_network_type(const QString& param);
  QString is_html_verified(const QString& param);
  QString get_html_content_hash(const QString& param);
  QString transfer(const QString& param);
  QString have_secure_app_data(const QString& param);
  QString drop_secure_app_data();
  QString get_secure_app_data(const QString& param);
  QString store_secure_app_data(const QString& param, const QString& password);
  QString set_master_password(const QString& param);
  QString check_master_password(const QString& param);
  QString get_app_data(const QString& param);
  QString store_app_data(const QString& param);
  QString get_default_user_dir(const QString& param);
  QString get_offers_ex(const QString& param);
  QString push_offer(const QString& param);
  QString cancel_offer(const QString& param);
  QString push_update_offer(const QString& param);
  QString get_alias_info_by_address(const QString& param);
  QString get_alias_info_by_name(const QString& param);
  QString get_all_aliases(const QString& param);
  QString request_alias_registration(const QString& param);
  QString request_alias_update(const QString& param);
  QString get_alias_coast(const QString& param);
  QString validate_address(const QString& param);
  QString resync_wallet(const QString& param);
  QString get_recent_transfers(const QString& param);
  QString get_mining_history(const QString& param);
  QString start_pos_mining(const QString& param);
  QString stop_pos_mining(const QString& param);
  QString set_log_level(const QString& param);
  QString get_log_level(const QString& param);
  QString set_enable_tor(const QString& param);
  QString webkit_launched_script(const QString& param);
  QString get_smart_wallet_info(const QString& param);
  QString restore_wallet(const QString& param);
  QString use_whitelisting(const QString& param);
  QString is_pos_allowed(const QString& param);
  QString store_to_file(const QString& path, const QString& buff);
  QString load_from_file(const QString& path);
  QString is_file_exist(const QString& path);
  QString get_mining_estimate(const QString& obj);
  QString backup_wallet_keys(const QString& obj);
  QString reset_wallet_password(const QString& param);
  QString is_wallet_password_valid(const QString& param);
  QString is_autostart_enabled(const QString& param);
  QString toggle_autostart(const QString& param);
  QString is_valid_restore_wallet_text(const QString& param);
  QString get_seed_phrase_info(const QString& param);
  QString print_text(const QString& param);
  QString print_log(const QString& param);
  QString set_clipboard(const QString& param);
  QString set_localization_strings(const QString str);
  QString get_clipboard(const QString& param);
  void message_box(const QString& msg);
  bool toggle_mining(const QString& param);
  QString get_exchange_last_top(const QString& params);
  QString get_tx_pool_info(const QString& param);
  QString get_default_fee(const QString& param);
  QString get_options(const QString& param);
  void bool_toggle_icon(const QString& param);
  QString add_custom_asset_id(const QString& param);
  QString remove_custom_asset_id(const QString& param);
  QString get_wallet_info(const QString& param);

  QString create_ionic_swap_proposal(const QString& param);
  QString get_ionic_swap_proposal_info(const QString& param);
  QString accept_ionic_swap_proposal(const QString& param);

  bool get_is_disabled_notifications(const QString& param);
  bool set_is_disabled_notifications(const bool& param);
  QString export_wallet_history(const QString& param);
  QString get_log_file(const QString& param);
  QString open_url_in_browser(const QString& param);
  QString setup_jwt_wallet_rpc(const QString& param);

  void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
  void tray_quit_requested(const QString& param);
  void on_menu_show(const QString& param);
  QString is_remnotenode_mode_preconfigured(const QString& param);
  QString start_backend(const QString& params);
  void show_notification(const QString& title, const QString& message);

  QString async_call(const QString& func_name, const QString& params);
  QString sync_call(const QString& func_name, const QString& params);
  QString async_call_2a(const QString& func_name, const QString& params1, const QString& params2);
  QString sync_call_2a(const QString& func_name, const QString& params1, const QString& params2);

  QString request_dummy(const QString& param);

  QString call_rpc(const QString& params);
  QString call_wallet_rpc(const QString& wallet_id, const QString& params);

signals:
  void quit_requested(QString str);
  void update_daemon_state(QString str);
  void update_wallet_status(QString str);
  void update_wallet_info(QString str);
  void money_transfer(QString str);
  void money_transfer_cancel(QString str);
  void wallet_sync_progress(QString str);
  void handle_internal_callback(QString str, QString callback_name);
  void update_pos_mining_text(QString str);
  void on_core_event(QString method_name);
  void set_options(QString str);
  void handle_deeplink_click(QString str);
  void handle_current_action_state(QString str);
  void dispatch_async_call_result(QString id, QString resp);

private:
  MainWindow& m_main_window;
};
