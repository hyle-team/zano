// Copyright (c) 2026 Zano Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "web_channel_bridge.h"

#include "mainwindow.h"

WebChannelBridge::WebChannelBridge(MainWindow& main_window, QObject* parent)
  : QObject(parent)
  , m_main_window(main_window)
{
  setObjectName(QStringLiteral("web_channel_bridge"));

  connect(&m_main_window, &MainWindow::quit_requested,
    this, &WebChannelBridge::quit_requested);
  connect(&m_main_window, &MainWindow::update_daemon_state,
    this, &WebChannelBridge::update_daemon_state);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::update_wallet_status),
    this, &WebChannelBridge::update_wallet_status);
  connect(&m_main_window, &MainWindow::update_wallet_info,
    this, &WebChannelBridge::update_wallet_info);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::money_transfer),
    this, &WebChannelBridge::money_transfer);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::money_transfer_cancel),
    this, &WebChannelBridge::money_transfer_cancel);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::wallet_sync_progress),
    this, &WebChannelBridge::wallet_sync_progress);
  connect(&m_main_window, &MainWindow::handle_internal_callback,
    this, &WebChannelBridge::handle_internal_callback);
  connect(&m_main_window, &MainWindow::update_pos_mining_text,
    this, &WebChannelBridge::update_pos_mining_text);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::on_core_event),
    this, &WebChannelBridge::on_core_event);
  connect(&m_main_window,
    static_cast<void (MainWindow::*)(QString)>(&MainWindow::set_options),
    this, &WebChannelBridge::set_options);
  connect(&m_main_window, &MainWindow::handle_deeplink_click,
    this, &WebChannelBridge::handle_deeplink_click);
  connect(&m_main_window, &MainWindow::handle_current_action_state,
    this, &WebChannelBridge::handle_current_action_state);
  connect(&m_main_window, &MainWindow::dispatch_async_call_result,
    this, &WebChannelBridge::dispatch_async_call_result);
}

#define ZANO_FORWARD_QSTRING_1(method_name) \
  QString WebChannelBridge::method_name(const QString& value) \
  { \
    return m_main_window.method_name(value); \
  }

ZANO_FORWARD_QSTRING_1(show_openfile_dialog)
ZANO_FORWARD_QSTRING_1(show_savefile_dialog)
ZANO_FORWARD_QSTRING_1(open_wallet)
ZANO_FORWARD_QSTRING_1(get_my_offers)
ZANO_FORWARD_QSTRING_1(get_fav_offers)
ZANO_FORWARD_QSTRING_1(generate_wallet)
ZANO_FORWARD_QSTRING_1(run_wallet)
ZANO_FORWARD_QSTRING_1(close_wallet)
ZANO_FORWARD_QSTRING_1(get_contracts)
ZANO_FORWARD_QSTRING_1(create_proposal)
ZANO_FORWARD_QSTRING_1(accept_proposal)
ZANO_FORWARD_QSTRING_1(release_contract)
ZANO_FORWARD_QSTRING_1(request_cancel_contract)
ZANO_FORWARD_QSTRING_1(accept_cancel_contract)
ZANO_FORWARD_QSTRING_1(on_request_quit)
ZANO_FORWARD_QSTRING_1(get_version)
ZANO_FORWARD_QSTRING_1(get_os_version)
ZANO_FORWARD_QSTRING_1(get_network_type)
ZANO_FORWARD_QSTRING_1(is_html_verified)
ZANO_FORWARD_QSTRING_1(get_html_content_hash)
ZANO_FORWARD_QSTRING_1(transfer)
ZANO_FORWARD_QSTRING_1(have_secure_app_data)
ZANO_FORWARD_QSTRING_1(get_secure_app_data)
ZANO_FORWARD_QSTRING_1(set_master_password)
ZANO_FORWARD_QSTRING_1(check_master_password)
ZANO_FORWARD_QSTRING_1(get_app_data)
ZANO_FORWARD_QSTRING_1(store_app_data)
ZANO_FORWARD_QSTRING_1(get_default_user_dir)
ZANO_FORWARD_QSTRING_1(get_offers_ex)
ZANO_FORWARD_QSTRING_1(push_offer)
ZANO_FORWARD_QSTRING_1(cancel_offer)
ZANO_FORWARD_QSTRING_1(push_update_offer)
ZANO_FORWARD_QSTRING_1(get_alias_info_by_address)
ZANO_FORWARD_QSTRING_1(get_alias_info_by_name)
ZANO_FORWARD_QSTRING_1(get_all_aliases)
ZANO_FORWARD_QSTRING_1(request_alias_registration)
ZANO_FORWARD_QSTRING_1(request_alias_update)
ZANO_FORWARD_QSTRING_1(get_alias_coast)
ZANO_FORWARD_QSTRING_1(validate_address)
ZANO_FORWARD_QSTRING_1(resync_wallet)
ZANO_FORWARD_QSTRING_1(get_recent_transfers)
ZANO_FORWARD_QSTRING_1(get_mining_history)
ZANO_FORWARD_QSTRING_1(start_pos_mining)
ZANO_FORWARD_QSTRING_1(stop_pos_mining)
ZANO_FORWARD_QSTRING_1(set_log_level)
ZANO_FORWARD_QSTRING_1(get_log_level)
ZANO_FORWARD_QSTRING_1(set_enable_tor)
ZANO_FORWARD_QSTRING_1(webkit_launched_script)
ZANO_FORWARD_QSTRING_1(get_smart_wallet_info)
ZANO_FORWARD_QSTRING_1(restore_wallet)
ZANO_FORWARD_QSTRING_1(use_whitelisting)
ZANO_FORWARD_QSTRING_1(is_pos_allowed)
ZANO_FORWARD_QSTRING_1(load_from_file)
ZANO_FORWARD_QSTRING_1(is_file_exist)
ZANO_FORWARD_QSTRING_1(get_mining_estimate)
ZANO_FORWARD_QSTRING_1(backup_wallet_keys)
ZANO_FORWARD_QSTRING_1(reset_wallet_password)
ZANO_FORWARD_QSTRING_1(is_wallet_password_valid)
ZANO_FORWARD_QSTRING_1(is_autostart_enabled)
ZANO_FORWARD_QSTRING_1(toggle_autostart)
ZANO_FORWARD_QSTRING_1(is_valid_restore_wallet_text)
ZANO_FORWARD_QSTRING_1(get_seed_phrase_info)
ZANO_FORWARD_QSTRING_1(print_text)
ZANO_FORWARD_QSTRING_1(print_log)
ZANO_FORWARD_QSTRING_1(set_clipboard)
ZANO_FORWARD_QSTRING_1(get_clipboard)
ZANO_FORWARD_QSTRING_1(get_exchange_last_top)
ZANO_FORWARD_QSTRING_1(get_tx_pool_info)
ZANO_FORWARD_QSTRING_1(get_default_fee)
ZANO_FORWARD_QSTRING_1(get_options)
ZANO_FORWARD_QSTRING_1(add_custom_asset_id)
ZANO_FORWARD_QSTRING_1(remove_custom_asset_id)
ZANO_FORWARD_QSTRING_1(get_wallet_info)
ZANO_FORWARD_QSTRING_1(create_ionic_swap_proposal)
ZANO_FORWARD_QSTRING_1(get_ionic_swap_proposal_info)
ZANO_FORWARD_QSTRING_1(accept_ionic_swap_proposal)
ZANO_FORWARD_QSTRING_1(export_wallet_history)
ZANO_FORWARD_QSTRING_1(get_log_file)
ZANO_FORWARD_QSTRING_1(open_url_in_browser)
ZANO_FORWARD_QSTRING_1(setup_jwt_wallet_rpc)
ZANO_FORWARD_QSTRING_1(is_remnotenode_mode_preconfigured)
ZANO_FORWARD_QSTRING_1(start_backend)
ZANO_FORWARD_QSTRING_1(request_dummy)
ZANO_FORWARD_QSTRING_1(call_rpc)

#undef ZANO_FORWARD_QSTRING_1

QString WebChannelBridge::drop_secure_app_data()
{
  return m_main_window.drop_secure_app_data();
}

QString WebChannelBridge::store_secure_app_data(const QString& param, const QString& password)
{
  return m_main_window.store_secure_app_data(param, password);
}

QString WebChannelBridge::store_to_file(const QString& path, const QString& buff)
{
  return m_main_window.store_to_file(path, buff);
}

QString WebChannelBridge::set_localization_strings(const QString str)
{
  return m_main_window.set_localization_strings(str);
}

void WebChannelBridge::message_box(const QString& msg)
{
  m_main_window.message_box(msg);
}

bool WebChannelBridge::toggle_mining(const QString& param)
{
  return m_main_window.toggle_mining(param);
}

void WebChannelBridge::bool_toggle_icon(const QString& param)
{
  m_main_window.bool_toggle_icon(param);
}

bool WebChannelBridge::get_is_disabled_notifications(const QString& param)
{
  return m_main_window.get_is_disabled_notifications(param);
}

bool WebChannelBridge::set_is_disabled_notifications(const bool& param)
{
  return m_main_window.set_is_disabled_notifications(param);
}

void WebChannelBridge::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  m_main_window.trayIconActivated(reason);
}

void WebChannelBridge::tray_quit_requested(const QString& param)
{
  m_main_window.tray_quit_requested(param);
}

void WebChannelBridge::on_menu_show(const QString& param)
{
  m_main_window.on_menu_show(param);
}

void WebChannelBridge::show_notification(const QString& title, const QString& message)
{
  m_main_window.show_notification(title, message);
}

QString WebChannelBridge::async_call(const QString& func_name, const QString& params)
{
  return m_main_window.async_call(func_name, params);
}

QString WebChannelBridge::sync_call(const QString& func_name, const QString& params)
{
  return m_main_window.sync_call(func_name, params);
}

QString WebChannelBridge::async_call_2a(const QString& func_name, const QString& params1, const QString& params2)
{
  return m_main_window.async_call_2a(func_name, params1, params2);
}

QString WebChannelBridge::sync_call_2a(const QString& func_name, const QString& params1, const QString& params2)
{
  return m_main_window.sync_call_2a(func_name, params1, params2);
}

QString WebChannelBridge::call_wallet_rpc(const QString& wallet_id, const QString& params)
{
  return m_main_window.call_wallet_rpc(wallet_id, params);
}
