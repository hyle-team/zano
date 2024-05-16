// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QPrinter>
#include <QPrintDialog>

#include "string_coding.h"
#include "gui_utils.h"
#include "notification_helper.h"
#include "common/config_encrypt_helper.h"
#include "currency_core/basic_kv_structs.h"

#define PREPARE_ARG_FROM_JSON(arg_type, var_name)   \
  arg_type var_name = AUTO_VAL_INIT(var_name); \
  view::api_response default_ar = AUTO_VAL_INIT(default_ar);  \
  LOG_PRINT_BLUE("[REQUEST]: " << param.toStdString(), LOG_LEVEL_3); \
if (!epee::serialization::load_t_from_json(var_name, param.toStdString())) \
{ \
  default_ar.error_code = API_RETURN_CODE_BAD_ARG;                 \
  return MAKE_RESPONSE(default_ar); \
}

template<typename T>
QString make_response(const T& r)
{
  std::string str = epee::serialization::store_t_to_json(r);
  LOG_PRINT_BLUE("[RESPONSE]: " << str, LOG_LEVEL_3);
  return str.c_str();
}

template<typename T>
QString make_response_dbg(const T& r, const std::string& location)
{
  std::string str = epee::serialization::store_t_to_json(r);
  LOG_PRINT_YELLOW("***** API RESPONSE from " << location << " : " << ENDL << str, LOG_LEVEL_0);
  return str.c_str();
}

#define PREPARE_RESPONSE(rsp_type, var_name)   view::api_response_t<rsp_type> var_name = AUTO_VAL_INIT(var_name) 
#define MAKE_RESPONSE(r) (r.error_code == API_RETURN_CODE_OK || r.error_code == API_RETURN_CODE_TRUE) ? make_response(r) : make_response_dbg(r, LOCATION_STR)

#define LOG_API_TIMING() const char* pfunc_call_name = LOCAL_FUNCTION_DEF__; LOG_PRINT_BLUE("[API:" << pfunc_call_name << "]-->>", LOG_LEVEL_0); uint64_t ticks_before_start = epee::misc_utils::get_tick_count(); \
  auto cb_leave = epee::misc_utils::create_scope_leave_handler([&ticks_before_start, &pfunc_call_name](){ \
  LOG_PRINT_BLUE("[API:" << pfunc_call_name << "]<<-- (" << epee::misc_utils::get_tick_count() - ticks_before_start << "ms)" << (epee::misc_utils::get_tick_count() - ticks_before_start  > 1000 ? "[!!!LONG CALL!!!]":""), LOG_LEVEL_0); \
  });

#define LOG_API_PARAMS(log_level) LOG_PRINT_BLUE(LOCAL_FUNCTION_DEF__ << "(" << param.toStdString() << ")", log_level)

#define   CATCH_ENTRY_FAIL_API_RESPONCE() } \
  catch (const std::exception& ex) \
  { \
    LOG_ERROR("Exception catched, ERROR:" << ex.what()); \
    PREPARE_RESPONSE(view::api_void, err_resp); \
    err_resp.error_code = API_RETURN_CODE_INTERNAL_ERROR; \
    return MAKE_RESPONSE(err_resp); \
  } \
  catch(...) \
  { \
    PREPARE_RESPONSE(view::api_void, err_resp); \
    err_resp.error_code = API_RETURN_CODE_INTERNAL_ERROR; \
    return MAKE_RESPONSE(err_resp); \
  }

#include "mainwindow.h"
// 
// void MediatorObject::from_html_to_c(const QString &text)
// {
//   from_c_to_html(text);
// }
// 
// template<typename Arg, typename R, typename C>
// struct InvokeWrapper {
//   R *receiver;
//   void (C::*memberFun)(Arg);
//   void operator()(Arg result) {
//     (receiver->*memberFun)(result);
//   }
// };
// 
// template<typename Arg, typename R, typename C>
// InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFun)(Arg))
// {
//   InvokeWrapper<Arg, R, C> wrapper = { receiver, memberFun };
//   return wrapper;
// }


std::wstring convert_to_lower_via_qt(const std::wstring& w)
{
	std::wstring r;
	return QString().fromStdWString(w).toLower().toStdWString();
}

MainWindow::MainWindow()
  : m_gui_deinitialize_done_1(false)
  , m_backend_stopped_2(false)
  , m_system_shutdown(false)
  , m_view(nullptr)
  , m_channel(nullptr)
  , m_ui_dispatch_id_counter(0)
{
#ifndef _MSC_VER
  //workaround for macos broken tolower from std, very dirty hack
  bc_services::set_external_to_low_converter(convert_to_lower_via_qt);
#endif
}

MainWindow::~MainWindow()
{
  m_backend.subscribe_to_core_events(nullptr);
  if (m_view)
  {
    m_view->page()->setWebChannel(nullptr);
    m_view = nullptr;
  }
  if (m_channel)
  {
    m_channel->deregisterObject(this);
    delete m_channel;
    m_channel = nullptr;
  }
  if (m_ipc_worker.joinable())
  {
    m_ipc_worker.join();
  }
}

void MainWindow::on_load_finished(bool ok)
{
  TRY_ENTRY();
  LOG_PRINT("MainWindow::on_load_finished(ok = " << (ok ? "true" : "false") << ")", LOG_LEVEL_0);
  CATCH_ENTRY2(void());
}

bool MainWindow::init_window()
{
  m_view = new QWebEngineView(this);
  m_channel = new QWebChannel(m_view->page());
  m_view->page()->setWebChannel(m_channel);

  QWidget* central_widget_to_be_set = m_view;
  double zoom_factor_test = 0.75;
  m_view->setZoomFactor(zoom_factor_test);

  std::string qt_dev_tools_option = m_backend.get_qt_dev_tools_option();
  if (!qt_dev_tools_option.empty())
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    std::vector<std::string> qt_dev_tools_option_parts;
    boost::split(qt_dev_tools_option_parts, qt_dev_tools_option, [](char c) { return c == ','; });
    
    Qt::Orientation orientation = Qt::Vertical;
    if (qt_dev_tools_option_parts.size() >= 1 && qt_dev_tools_option_parts[0] == "horizontal")
      orientation = Qt::Horizontal;

    double zoom_factor = 1.3;
    if (qt_dev_tools_option_parts.size() >= 2)
      epee::string_tools::get_xtype_from_string(zoom_factor, qt_dev_tools_option_parts[1]);

    QSplitter* spliter = new QSplitter(orientation);
    spliter->addWidget(m_view);
    QWebEngineView* inspector = new QWebEngineView();
    spliter->addWidget(inspector);
    m_view->page()->setDevToolsPage(inspector->page());
    inspector->setZoomFactor(zoom_factor);

    spliter->setCollapsible(0, false);
    spliter->setCollapsible(1, false);

    QList<int> Sizes;
    Sizes.append(0.5 * m_view->sizeHint().height());
    Sizes.append(0.5 * m_view->sizeHint().height());
    spliter->setSizes(Sizes);

    central_widget_to_be_set = spliter;
#else
    LOG_ERROR("Qt Dev Tool is not available for this Qt version, try building with Qt 5.11.0 or higher");
#endif
  }

  // register QObjects to be exposed to JavaScript
  m_channel->registerObject(QStringLiteral("mediator_object"), this);

  connect(m_view, SIGNAL(loadFinished(bool)), SLOT(on_load_finished(bool)));

  setCentralWidget(central_widget_to_be_set);
  //this->setMouseTracking(true);

  m_view->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  m_view->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  m_view->page()->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

  m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  m_view->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  m_view->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);

  m_localization.resize(localization_id_couter);
  m_localization[localization_id_quit] = "Quit";
  m_localization[localization_id_is_received] = " is received";
  m_localization[localization_id_is_confirmed] = " is confirmed";
  m_localization[localization_id_income_transfer_unconfirmed] = "Income transfer (unconfirmed)";
  m_localization[localization_id_income_transfer_confirmed] = "Income transfer confirmed";
  m_localization[localization_id_locked] = "(locked)";
  m_localization[localization_id_mined] = "(mined)";
  m_localization[localization_id_minimized_title] = "Zano app is minimized to tray";
  m_localization[localization_id_minimized_text] = "You can restore it with double-click or context menu";
  m_localization[localization_id_tray_menu_show] = "localization_id_tray_menu_show";
  m_localization[localization_id_tray_menu_minimize] = "localization_id_tray_menu_minimize";

  return true;
}

QString MainWindow::get_default_user_dir(const QString& param)
{
  TRY_ENTRY();
  return tools::get_default_user_dir().c_str();
  CATCH_ENTRY_FAIL_API_RESPONCE();
}


bool MainWindow::toggle_mining(const QString& param)
{
  TRY_ENTRY();
  m_backend.toggle_pos_mining();
  return true;
  CATCH_ENTRY2(false);
}

QString MainWindow::get_exchange_last_top(const QString& params)
{
  TRY_ENTRY();
  return QString();
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_tx_pool_info(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_POOL_INFO::response, ar);
  ar.error_code = m_backend.get_tx_pool_info(ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::request_dummy(const QString& param)
{
  static int code_ = 0;
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_POOL_INFO::response, ar);
  if (code_ == 2)
  {
    code_ = -1;
    ar.error_code = API_RETURN_CODE_CORE_BUSY;
  }
  else
  {
    ar.error_code = API_RETURN_CODE_OK;
  }

  ++code_;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::call_rpc(const QString& params)
{
  TRY_ENTRY();

  if (!m_backend.is_core_initialized())
  {
    epee::json_rpc::error_response rsp;
    rsp.jsonrpc = "2.0";
    rsp.error.code = -1;
    rsp.error.message = API_RETURN_CODE_CORE_BUSY;
    return QString::fromStdString(epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp)));
  }

  epee::net_utils::http::http_request_info query_info = AUTO_VAL_INIT(query_info);
  epee::net_utils::http::http_response_info response_info  = AUTO_VAL_INIT(response_info);
  currency::core_rpc_server::connection_context dummy_context(RPC_INTERNAL_UI_CONTEXT, 0, 0, true);

  query_info.m_URI = "/json_rpc";
  query_info.m_body = params.toStdString();
  
  m_backend.get_rpc_server().handle_http_request(query_info, response_info, dummy_context);
  if (response_info.m_response_code != 200)
  {
    epee::json_rpc::error_response rsp; 
    rsp.jsonrpc = "2.0"; 
    rsp.error.code = response_info.m_response_code;
    rsp.error.message = response_info.m_response_comment;
    return QString::fromStdString(epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp)));
  }
  return QString::fromStdString(response_info.m_body);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::call_wallet_rpc(const QString& wallet_id_str, const QString& params)
{
  TRY_ENTRY();

  if (!m_backend.is_core_initialized())
  {
    epee::json_rpc::error_response rsp;
    rsp.jsonrpc = "2.0";
    rsp.error.code = -1;
    rsp.error.message = API_RETURN_CODE_CORE_BUSY;
    return QString::fromStdString(epee::serialization::store_t_to_json(static_cast<epee::json_rpc::error_response&>(rsp)));
  }

  uint64_t wallet_id = std::stoull(wallet_id_str.toStdString());

  return QString::fromStdString(m_backend.invoke(wallet_id, params.toStdString()));
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_default_fee(const QString& param)
{
  TRY_ENTRY();
  return QString(std::to_string(m_backend.get_default_fee()).c_str());
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_options(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(view::gui_options, ar);
  m_backend.get_gui_options(ar.response_data);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

void MainWindow::tray_quit_requested(const QString& param)
{
  TRY_ENTRY();
  LOG_PRINT_MAGENTA("[GUI]->[HTML] tray_quit_requested", LOG_LEVEL_0);  
  emit quit_requested("{}");
  CATCH_ENTRY2(void());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  TRY_ENTRY();
  LOG_PRINT_L0("[GUI] CLOSE EVENT");
   CHECK_AND_ASSERT_MES(m_gui_deinitialize_done_1 == m_backend_stopped_2, void(), "m_gui_deinitialize_done_1 != m_backend_stopped_2, m_gui_deinitialize_done_1 = " << m_gui_deinitialize_done_1 
     << "m_backend_stopped_2 = " << m_backend_stopped_2);


   if (m_system_shutdown && !m_gui_deinitialize_done_1)
   {
     LOG_PRINT_MAGENTA("Shutting down without waiting response from html", LOG_LEVEL_0);
     //Usually QtWebEngineProcess.exe already killed at this moment, so we won't get response from html.
     m_gui_deinitialize_done_1 = true;
     m_backend.send_stop_signal();
   }
   else if (m_gui_deinitialize_done_1 && m_backend_stopped_2)
   {
     store_pos(true);
     store_app_config();
     event->accept();
   }
   else
   {
     event->ignore();
     //m_quit_requested = true;
     LOG_PRINT_L0("[GUI]->[HTML] quit_requested");
     emit quit_requested("{}");
   }
   CATCH_ENTRY2(void());
}

std::string state_to_text(int s)
{
  TRY_ENTRY();
  std::string res = epee::string_tools::int_to_hex(s);
  res += "(";
  if (s & Qt::WindowMinimized)
    res += " WindowMinimized";
  if (s & Qt::WindowMaximized)
    res += " WindowMaximized";
  if (s & Qt::WindowFullScreen)
    res += " WindowFullScreen";
  if (s & Qt::WindowActive)
    res += " WindowActive";
  res += ")";

  return res;
  CATCH_ENTRY2("");
}

void MainWindow::changeEvent(QEvent *e)
{
  TRY_ENTRY();
  switch (e->type())
  {
  case QEvent::WindowStateChange:
  {
    QWindowStateChangeEvent* event = static_cast< QWindowStateChangeEvent* >(e);
    qDebug() << "Old state: " << state_to_text(event->oldState()).c_str() << ", new state: " << state_to_text(this->windowState()).c_str();

    if (event->oldState() & Qt::WindowMinimized && !(this->windowState() & Qt::WindowMinimized))
    {
      qDebug() << "Window restored (to normal or maximized state)!";
      if (m_tray_icon)
      {
        //QTimer::singleShot(250, this, SLOT(show()));
      }
      restore_pos();
    }
    else if (!(event->oldState() & Qt::WindowMinimized) && (this->windowState() & Qt::WindowMinimized))
    {
      store_pos();
      qDebug() << "Window minimized";
      show_notification(m_localization[localization_id_minimized_title], m_localization[localization_id_minimized_text]);
    }
    else if (!(event->oldState() & Qt::WindowFullScreen) && (this->windowState() & Qt::WindowFullScreen))
    {
      //maximize
      store_window_pos();
      this->update();
    }
    else if ((event->oldState() & Qt::WindowFullScreen) && !(this->windowState() & Qt::WindowFullScreen))
    {
      //restore
      this->update();
    }

    break;
  }
  default:
    break;
  }

  QWidget::changeEvent(e);
  CATCH_ENTRY2(void());
}

bool MainWindow::store_app_config()
{
  TRY_ENTRY();
  std::string conf_path = m_backend.get_config_folder() + "/" + GUI_INTERNAL_CONFIG2;
  LOG_PRINT_L0("storing gui internal config to " << conf_path);
  CHECK_AND_ASSERT_MES(epee::serialization::store_t_to_json_file(m_config, conf_path), false, "failed to store gui internal config");
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::load_app_config()
{
  TRY_ENTRY();
  std::string conf_path = m_backend.get_config_folder() + "/" + GUI_INTERNAL_CONFIG2;
  LOG_PRINT_L0("loading gui internal config from " << conf_path);
  bool r = epee::serialization::load_t_from_json_file(m_config, conf_path);
  LOG_PRINT_L0("gui internal config " << (r ? "loaded ok" : "was not loaded"));
  return r;
  CATCH_ENTRY2(false);
}

bool MainWindow::init(const std::string& html_path)
{
  TRY_ENTRY();
  //QtWebEngine::initialize();
  init_tray_icon(html_path);
  set_html_path(html_path);
  m_threads_pool.init(2);
  m_backend.subscribe_to_core_events(this);

  bool r = QSslSocket::supportsSsl();
  if (r)
  {
    LOG_PRINT_GREEN("[Support SSL]: YES", LOG_LEVEL_0);
  }
  else
  {
//    QMessageBox::question(this, "OpenSSL support disabled.", "OpenSSL support disabled.",
//      QMessageBox::Ok);
    LOG_PRINT_RED("[Support SSL]: NO", LOG_LEVEL_0);
  }

  //----
  this->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
  m_view->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  return true;
  CATCH_ENTRY2(false);
}

void MainWindow::on_menu_show(const QString& param)
{
  TRY_ENTRY();
  qDebug() << "Context menu: show()";
  this->show();
  this->activateWindow();
  CATCH_ENTRY2(void());
}

void MainWindow::init_tray_icon(const std::string& html_path)
{
  TRY_ENTRY();
  if (!QSystemTrayIcon::isSystemTrayAvailable())
  {
    LOG_PRINT_L0("System tray is unavailable");
    return;
  }


  m_restore_action = std::unique_ptr<QAction>(new QAction(tr("&Restore"), this));
  connect(m_restore_action.get(), SIGNAL(triggered()), this, SLOT(on_menu_show()));

  m_quit_action = std::unique_ptr<QAction>(new QAction(tr("&Quit"), this));
  connect(m_quit_action.get(), SIGNAL(triggered()), this, SLOT(tray_quit_requested()));

  m_minimize_action = std::unique_ptr<QAction>(new QAction(tr("minimizeAction"), this));
  connect(m_minimize_action.get(), SIGNAL(triggered()), this, SLOT(showMinimized()));

  m_tray_icon_menu = std::unique_ptr<QMenu>(new QMenu(this));
  m_tray_icon_menu->addAction(m_minimize_action.get());
  //m_tray_icon_menu->addAction(m_restore_action.get());
  m_tray_icon_menu->addSeparator();
  m_tray_icon_menu->addAction(m_quit_action.get());

  m_tray_icon = std::unique_ptr<QSystemTrayIcon>(new QSystemTrayIcon(this));
  m_tray_icon->setContextMenu(m_tray_icon_menu.get());

  //setup icon
#ifdef TARGET_OS_MAC
  m_normal_icon_path = html_path + "/files/app22macos.png"; // X11 tray icon size is 22x22
  m_blocked_icon_path = html_path + "/files/app22macos_blocked.png"; // X11 tray icon size is 22x22
#else
  m_normal_icon_path = html_path + "/files/app22windows.png"; // X11 tray icon size is 22x22
  m_blocked_icon_path = html_path + "/files/app22windows_blocked.png"; // X11 tray icon size
#endif
                                                                      //setWindowIcon(QIcon(iconPath.c_str()));
  QIcon qi( QString::fromWCharArray(epee::string_encoding::utf8_to_wstring(m_normal_icon_path).c_str()) );
  qi.setIsMask(true);
  m_tray_icon->setIcon(qi);
  m_tray_icon->setToolTip(CURRENCY_NAME_BASE);
  connect(m_tray_icon.get(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
    this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
  m_tray_icon->show();
  CATCH_ENTRY2(void());
}

void MainWindow::bool_toggle_icon(const QString& param)
{
  TRY_ENTRY();
  std::string path;

  if (param == "blocked")
    path = m_blocked_icon_path;
  else
    path = m_normal_icon_path;

  QIcon qi( QString::fromWCharArray(epee::string_encoding::utf8_to_wstring(path).c_str()) );
  qi.setIsMask(true);
  m_tray_icon->setIcon(qi);
  CATCH_ENTRY2(void());
}

QString MainWindow::get_log_file(const QString& param)
{
  TRY_ENTRY();
  std::string buff;
  epee::file_io_utils::load_last_n_from_file_to_string(log_space::log_singletone::get_actual_log_file_path(), 1000000, buff);
  return QString::fromStdString(buff);
  CATCH_ENTRY2("");
}

void  MainWindow::store_window_pos()
{
  TRY_ENTRY();
  QPoint pos = this->pos();
  QSize sz = this->size();
  m_config.m_window_position.first = pos.x();
  m_config.m_window_position.second = pos.y();
  m_config.m_window_size.first = sz.height();
  m_config.m_window_size.second = sz.width();

  CATCH_ENTRY2(void());
}
void MainWindow::store_pos(bool consider_showed)
{
  TRY_ENTRY();
  m_config.is_maximazed = this->isMaximized();
  //here position supposed to be filled from last unserialize  or filled on maximize handler
  if (!m_config.is_maximazed)
    store_window_pos();
  if (consider_showed)
    m_config.is_showed = this->isVisible();

  CATCH_ENTRY2(void());
}
void MainWindow::restore_pos(bool consider_showed)
{
  TRY_ENTRY();
  if (consider_showed)
  {
    if (m_config.is_showed)
      this->showNormal();
    else
      this->showMinimized();
  }

  if (m_config.is_maximazed)
  {
    this->setWindowState(windowState() | Qt::WindowMaximized);
  }
  else
  {
    QPoint point = QApplication::desktop()->screenGeometry().bottomRight();
    if (m_config.m_window_position.first + m_config.m_window_size.second > point.x() ||
      m_config.m_window_position.second + m_config.m_window_size.first > point.y()
      )
    {
      QSize sz = AUTO_VAL_INIT(sz);
      sz.setHeight(770);
      sz.setWidth(1200);
      this->resize(sz);
      store_window_pos();
      //reset position(screen changed or other reason)
    }
    else
    {
      QPoint pos = AUTO_VAL_INIT(pos);
      QSize sz = AUTO_VAL_INIT(sz);
      pos.setX(m_config.m_window_position.first);
      pos.setY(m_config.m_window_position.second);
      sz.setHeight(m_config.m_window_size.first);
      sz.setWidth(m_config.m_window_size.second);
      this->move(pos);
      this->resize(sz);
    }
  }

  CATCH_ENTRY2(void());
}
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
  TRY_ENTRY();
  if (reason == QSystemTrayIcon::ActivationReason::Trigger)
  {
    if ( !(this->windowState() & Qt::WindowMinimized))
    {
      showMinimized();
    }
    else
    {
      showNormal();
      activateWindow();
    }


  }
  CATCH_ENTRY2(void());
}

void MainWindow::load_file(const QString &fileName)
{
  TRY_ENTRY();
  LOG_PRINT_L0("Loading html from path: " << fileName.toStdString());
  QUrl url = QUrl::fromLocalFile(QFileInfo(fileName).absoluteFilePath());
  m_view->load(url);
  CATCH_ENTRY2(void());
}

QString MainWindow::set_clipboard(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(param);
  return API_RETURN_CODE_OK;
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::get_clipboard(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  QClipboard *clipboard = QApplication::clipboard();
  return clipboard->text();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::on_request_quit(const QString& param)
{
  TRY_ENTRY();
  LOG_PRINT_MAGENTA("[HTML]->[GUI] on_request_quit", LOG_LEVEL_0);
  m_gui_deinitialize_done_1 = true;
  m_backend.send_stop_signal();

  return API_RETURN_CODE_OK;
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

bool MainWindow::do_close()
{
  TRY_ENTRY();
  this->close();
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::show_inital()
{
  TRY_ENTRY();
  if (load_app_config())
    restore_pos(true);
  else
  {
    m_config = AUTO_VAL_INIT(m_config);
    this->show();
    QSize sz = AUTO_VAL_INIT(sz);
    sz.setHeight(770);
    sz.setWidth(1200);
    this->resize(sz);
    store_window_pos();
    m_config.is_maximazed = false;
    m_config.is_showed = true;
    m_config.disable_notifications = false;
  }
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::on_backend_stopped()
{
  TRY_ENTRY();
  LOG_PRINT_L0("[BACKEND]->[GUI] on_backend_stopped");
  m_backend_stopped_2 = true;
  //m_deinitialize_done = true;
//  if (m_quit_requested)
//  {

    /*bool r = */QMetaObject::invokeMethod(this, "do_close", Qt::QueuedConnection);
// }
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::update_daemon_status(const view::daemon_status_info& info)
{
  TRY_ENTRY();
  //this->update_daemon_state(info);
  std::string json_str;
  epee::serialization::store_t_to_json(info, json_str);

  //lifehack
  if (m_last_update_daemon_status_json == json_str)
    return true;

  LOG_PRINT_L0("SENDING SIGNAL -> [update_daemon_state] " << info.daemon_network_state);
  //this->update_daemon_state(json_str.c_str());
  QMetaObject::invokeMethod(this, "update_daemon_state", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  m_last_update_daemon_status_json = json_str;
  return true;
  CATCH_ENTRY2(false);
}


bool MainWindow::show_msg_box(const std::string& message)
{
  TRY_ENTRY();
  QMessageBox::information(this, "Error", message.c_str(), QMessageBox::Ok);
  return true;
  CATCH_ENTRY2(false);
}

void qt_log_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray local_msg = msg.toLocal8Bit();
    const char* msg_type = "";
    switch (type)
    {
    case QtDebugMsg:    msg_type = "DEBG "; break;
    case QtInfoMsg:     msg_type = "INFO "; break;
    case QtWarningMsg:  msg_type = "WARN "; break;
    case QtCriticalMsg: msg_type = "CRIT "; break;
    case QtFatalMsg:    msg_type = "FATAL "; break;
    }

    if (context.file == nullptr && context.function == nullptr)
    {
      // no debug info
      LOG_PRINT("[QT] " << msg_type << local_msg.constData(), LOG_LEVEL_0);
    }
    else
    {
      // some debug info
      LOG_PRINT("[QT] " << msg_type << local_msg.constData() << " @ " << (context.file ? context.file : "") << ":" << context.line << ", " << (context.function ? context.function : ""), LOG_LEVEL_0);
    }
}

bool MainWindow::remove_ipc()
{
  try {
    boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
  }
  catch (...)
  {
  }
  return true;
}
 

bool MainWindow::init_ipc_server()
{

  //in case previous instance wasn't close graceful, ipc channel will remain open and new creation will fail, so we 
  //trying to close it anyway before open, to make sure there are no dead channels. If there are another running instance, it wom't 
  //let channel to close, so it will fail later on creating channel
  remove_ipc();
#define GUI_IPC_BUFFER_SIZE  10000
  try {
    //Create a message queue.
    std::shared_ptr<boost::interprocess::message_queue> pmq(new boost::interprocess::message_queue(boost::interprocess::create_only //only create
      , GUI_IPC_MESSAGE_CHANNEL_NAME           //name
      , 100                                    //max message number
      , GUI_IPC_BUFFER_SIZE                    //max message size
    ));

    m_ipc_worker = std::thread([this, pmq]()
    {
      //m_ipc_worker;
      try
      {
        unsigned int priority = 0;
        boost::interprocess::message_queue::size_type recvd_size = 0;

        while (m_gui_deinitialize_done_1 == false)
        {
          std::string buff(GUI_IPC_BUFFER_SIZE, ' ');
          bool data_received = pmq->timed_receive((void*)buff.data(), GUI_IPC_BUFFER_SIZE, recvd_size, priority, boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time()) + boost::posix_time::milliseconds(1000));
          if (data_received && recvd_size != 0)
          {
            buff.resize(recvd_size, '*');
            handle_ipc_event(buff);//todo process token
          }
        }        
        remove_ipc();
        LOG_PRINT_L0("IPC Handling thread finished");
      }
      catch (const std::exception& ex)
      {
        remove_ipc();
        boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
        LOG_ERROR("Failed to receive IPC que: " << ex.what());
      }

      catch (...)
      {
        remove_ipc();
        LOG_ERROR("Failed to receive IPC que: unknown exception");
      }
    });
  }
  catch(const std::exception& ex)
  {
    boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
    LOG_ERROR("Failed to initialize IPC que: " << ex.what());
    return false;
  }

  catch (...)
  {
    boost::interprocess::message_queue::remove(GUI_IPC_MESSAGE_CHANNEL_NAME);
    LOG_ERROR("Failed to initialize IPC que: unknown exception");
    return false;
  }
  return true;
}


bool MainWindow::handle_ipc_event(const std::string& arguments)
{
  std::string zzz = std::string("Received IPC: ") + arguments.c_str();
  std::cout << zzz;//message_box(zzz.c_str());

  handle_deeplink_click(arguments.c_str());

  return true;
}

bool MainWindow::handle_deeplink_params_in_commandline()
{
  std::string deep_link_params = command_line::get_arg(m_backend.get_arguments(), command_line::arg_deeplink);

  try {
    boost::interprocess::message_queue mq(boost::interprocess::open_only, GUI_IPC_MESSAGE_CHANNEL_NAME);
    mq.send(deep_link_params.data(), deep_link_params.size(), 0);
    return false;
  }
  catch (...)
  {
    //ui not launched yet
    return true;
  }
}

bool MainWindow::init_backend(int argc, char* argv[])
{

  TRY_ENTRY();
  std::string command_line_fail_details;
  if (!m_backend.init_command_line(argc, argv, command_line_fail_details))
  {
    this->show_msg_box(command_line_fail_details);
    return false;
  }

  if (command_line::has_arg(m_backend.get_arguments(), command_line::arg_deeplink))
  {
    if (!handle_deeplink_params_in_commandline())
      return false;
  }

  if (!init_window())
  {
    this->show_msg_box("Failed to main screen launch, check logs for the more detais.");
    return false;
  }

  if (!m_backend.init(this))
  {
    this->show_msg_box("Failed to initialize backend, check debug logs for more details.");
    return false;
  }



  if (m_backend.is_qt_logs_enabled())
  {
    qInstallMessageHandler(qt_log_message_handler);
    QLoggingCategory::setFilterRules("*=true"); // enable all logs
  }

  if (!init_ipc_server())
  {
    this->show_msg_box("Failed to initialize IPC server, check debug logs for more details.");
    return false;
  }

  return true;
  CATCH_ENTRY2(false);
}

QString    MainWindow::is_remnotenode_mode_preconfigured(const QString& param)
{
  TRY_ENTRY();
  return API_RETURN_CODE_FALSE;
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::start_backend(const QString& params)
{
  TRY_ENTRY();
  view::api_response ar = AUTO_VAL_INIT(ar);

  bool r = m_backend.start();
  if (!r)
  {
    ar.error_code = API_RETURN_CODE_INTERNAL_ERROR;
    return MAKE_RESPONSE(ar);
  }
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::sync_call(const QString& func_name, const QString& params)
{
  if (func_name == "transfer")
  {
    return this->transfer(params);
  }
  else if (func_name == "test_call")
  {
    return params;
  }
  else
  {
    return QString(QString() + "{ \"status\": \"Method '" + func_name  + "' not found\"}");
  }
}

QString MainWindow::async_call(const QString& func_name, const QString& params)
{

  uint64_t job_id = m_ui_dispatch_id_counter++;
  QString method_name = func_name;
  QString argements = params;

  auto async_callback = [this, method_name, argements, job_id]()
  {
    QString res_str = this->sync_call(method_name, argements);
    this->dispatch_async_call_result(std::to_string(job_id).c_str(), res_str);  //general function
  };

  m_threads_pool.add_job(async_callback);
  LOG_PRINT_L2("[UI_ASYNC_CALL]: started " << method_name.toStdString() << ", job id: " << job_id);
  return QString::fromStdString(std::string("{ \"job_id\": ") + std::to_string(job_id) + "}");
}


bool MainWindow::update_wallet_status(const view::wallet_status_info& wsi)
{
  TRY_ENTRY();
  m_wallet_states->operator [](wsi.wallet_id) = wsi.wallet_state;
  
  std::string json_str_pub;
  epee::serialization::store_t_to_json(static_cast<const view::wallet_status_info_base&>(wsi), json_str_pub, 0);
  LOG_PRINT_L0(get_wallet_log_prefix(wsi.wallet_id) + "SENDING SIGNAL -> [update_wallet_status]:" << std::endl << json_str_pub);
  
  std::string json_str;
  epee::serialization::store_t_to_json(wsi, json_str, 0);
  QMetaObject::invokeMethod(this, "update_wallet_status", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::set_options(const view::gui_options& opt)
{
  TRY_ENTRY();
  std::string json_str;
  epee::serialization::store_t_to_json(opt, json_str, 0);
  LOG_PRINT_L0("SENDING SIGNAL -> [set_options]:" << std::endl << json_str);
  QMetaObject::invokeMethod(this, "set_options", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::update_tor_status(const view::current_action_status& opt)
{
  TRY_ENTRY();
  std::string json_str;
  epee::serialization::store_t_to_json(opt, json_str, 0);
  LOG_PRINT_L0("SENDING SIGNAL -> [HANDLE_CURRENT_ACTION_STATE]:" << std::endl << json_str);
  QMetaObject::invokeMethod(this, "handle_current_action_state", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
  TRY_ENTRY();
#ifdef WIN32
  MSG *msg = static_cast< MSG * >(message);
  if (msg->message == WM_QUERYENDSESSION)
  {
    m_system_shutdown = true;
    LOG_PRINT_MAGENTA("SYSTEM SHUTDOWN", LOG_LEVEL_0);
  }
#endif
  return false;
  CATCH_ENTRY2(false);
}

bool MainWindow::get_is_disabled_notifications(const QString& param)
{
  return m_config.disable_notifications;
}
bool MainWindow::set_is_disabled_notifications(const bool& param)
{
  m_config.disable_notifications = param;
  return m_config.disable_notifications;
}
QString   MainWindow::export_wallet_history(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::export_wallet_info, ewi);
  PREPARE_RESPONSE(view::api_response, ar);
  ar.error_code = m_backend.export_wallet_history(ewi);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}
bool MainWindow::update_wallets_info(const view::wallets_summary_info& wsi)
{
  TRY_ENTRY();
  std::string json_str;
  epee::serialization::store_t_to_json(wsi, json_str, 0);
  LOG_PRINT_L0("SENDING SIGNAL -> [update_wallets_info]"<< std::endl << json_str );
  
  QMetaObject::invokeMethod(this, "update_wallets_info", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::money_transfer(const view::transfer_event_info& tei)
{
  TRY_ENTRY();
  std::string json_str;
  epee::serialization::store_t_to_json(tei, json_str, 0);

  LOG_PRINT_L0(get_wallet_log_prefix(tei.wallet_id) + "SENDING SIGNAL -> [money_transfer]" << std::endl << json_str);
  //this->money_transfer(json_str.c_str());
  QMetaObject::invokeMethod(this, "money_transfer", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));
  if (m_config.disable_notifications)
    return true;


  if (!m_tray_icon)
    return true;
  if (tei.ti.has_outgoing_entries())
    return true;
  if (!tei.ti.get_native_amount())
    return true;
//  if (tei.ti.is_mining && m_wallet_states->operator [](tei.wallet_id) != view::wallet_status_info::wallet_state_ready)
//    return true;

//don't show unconfirmed tx
  if (tei.ti.height == 0)
    return true;
  if (tei.is_wallet_in_sync_process)
  {
    //don't show notification if it long sync process(mmight cause system freeze)
    return true;
  }

  auto amount_str = currency::print_money_brief(tei.ti.get_native_amount()); //@#@ add handling of assets
  std::string title, msg;
  if (tei.ti.height == 0) // unconfirmed tx
  {
    msg = amount_str + " " + CURRENCY_NAME_ABR + " " + m_localization[localization_id_is_received];
    title = m_localization[localization_id_income_transfer_unconfirmed];
  }
  else
  {
    msg = amount_str + " " + CURRENCY_NAME_ABR + " " + m_localization[localization_id_is_confirmed];
    title = m_localization[localization_id_income_transfer_confirmed];
  }
  if (tei.ti.is_mining)
    msg += m_localization[localization_id_mined];
  else if (tei.ti.unlock_time)
    msg += m_localization[localization_id_locked];

  
  show_notification(title, msg);

  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::money_transfer_cancel(const view::transfer_event_info& tei)
{
  TRY_ENTRY();
  std::string json_str;
  epee::serialization::store_t_to_json(tei, json_str, 0);

  LOG_PRINT_L0(get_wallet_log_prefix(tei.wallet_id) + "SENDING SIGNAL -> [money_transfer_cancel]");
  //this->money_transfer_cancel(json_str.c_str());
  QMetaObject::invokeMethod(this, "money_transfer_cancel", Qt::QueuedConnection, Q_ARG(QString, json_str.c_str()));

  return true;

  CATCH_ENTRY2(false);
}
bool MainWindow::wallet_sync_progress(const view::wallet_sync_progres_param& p)
{
  TRY_ENTRY();
  LOG_PRINT_L2(get_wallet_log_prefix(p.wallet_id) + "SENDING SIGNAL -> [wallet_sync_progress]" << " wallet_id: " << p.wallet_id << ": " << p.progress << "%");
  //this->wallet_sync_progress(epee::serialization::store_t_to_json(p).c_str());
  QMetaObject::invokeMethod(this, "wallet_sync_progress", Qt::QueuedConnection, Q_ARG(QString, epee::serialization::store_t_to_json(p, 0).c_str()));
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::set_html_path(const std::string& path)
{
  TRY_ENTRY();
  //init_tray_icon(path);
#ifdef _MSC_VER
  QString url = QString::fromUtf8(path.c_str()) + "/index.html";
  load_file(url);
#else
//  load_file(QString((std::string("file://") + path + "/index.html").c_str()));
  load_file(QString((std::string("") + path + "/index.html").c_str()));
#endif
  return true;
  CATCH_ENTRY2(false);
}

bool MainWindow::pos_block_found(const currency::block& block_found)
{
  TRY_ENTRY();
  std::stringstream ss;
  ss << "Found Block h = " << currency::get_block_height(block_found);
  LOG_PRINT_L0("SENDING SIGNAL -> [update_pos_mining_text]");
  //this->update_pos_mining_text(ss.str().c_str());
  QMetaObject::invokeMethod(this, "update_pos_mining_text", Qt::QueuedConnection, Q_ARG(QString, ss.str().c_str()));
  return true;
  CATCH_ENTRY2(false);
}

QString MainWindow::get_version(const QString& param)
{
  TRY_ENTRY();
  return PROJECT_VERSION_LONG;
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_os_version(const QString& param)
{
  TRY_ENTRY();
  return tools::get_os_version_string().c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::get_network_type(const QString& param)
{
#if defined(TESTNET)
  return "testnet";
#else
  return "mainnet";
#endif
}

QString MainWindow::get_alias_coast(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(currency::struct_with_one_t_type<std::string>, lvl);
  view::get_alias_coast_response resp;
  resp.error_code = m_backend.get_alias_coast(lvl.v, resp.coast);
  return epee::serialization::store_t_to_json(resp, 0).c_str();
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::set_localization_strings(const QString param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::set_localization_request, lr);
  view::api_response resp;

  if (lr.strings.size()  < localization_id_couter)
  {
    LOG_ERROR("Wrong localization size: " << lr.strings.size() << ", expected size at least " << localization_id_couter);
    resp.error_code = API_RETURN_CODE_FAIL;
  }
  else
  {
    m_localization = lr.strings;
    if(m_quit_action)
      m_quit_action->setText(QString::fromStdString(m_localization[localization_id_quit]));
    if(m_restore_action)
      m_restore_action->setText(QString::fromStdString(m_localization[localization_id_tray_menu_show]));
    if(m_minimize_action)
      m_minimize_action->setText(QString::fromStdString(m_localization[localization_id_tray_menu_minimize]));
    resp.error_code = API_RETURN_CODE_OK;
    LOG_PRINT_L0("New localization set, language title: " << lr.language_title << ", strings " << lr.strings.size());
  }
  return epee::serialization::store_t_to_json(resp, 0).c_str();
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::request_alias_registration(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::request_alias_param>("request_alias_registration", param, [this](const view::request_alias_param& tp, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::request_alias_param, tp);
  PREPARE_RESPONSE(view::transfer_response, ar);

  //  view::transfer_response tr = AUTO_VAL_INIT(tr);
  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  ar.error_code = m_backend.request_alias_registration(tp.alias, tp.wallet_id, tp.fee, res_tx, tp.reward);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);


  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::request_alias_update(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::request_alias_param, tp);
  PREPARE_RESPONSE(view::transfer_response, ar);

  //  view::transfer_response tr = AUTO_VAL_INIT(tr);
  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  ar.error_code = m_backend.request_alias_update(tp.alias, tp.wallet_id, tp.fee, res_tx, tp.reward);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);


  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::transfer(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::transfer_params>("transfer", json_transfer_object, [this](const view::transfer_params& tp, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::transfer_params, tp);
  PREPARE_RESPONSE(view::transfer_response, ar);

  if (!tp.destinations.size())
  {
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return MAKE_RESPONSE(ar);
  }

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);
  ar.error_code = m_backend.transfer(tp.wallet_id, tp, res_tx);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);

  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

void MainWindow::message_box(const QString& msg)
{
  TRY_ENTRY();
  show_msg_box(msg.toStdString());
  CATCH_ENTRY2(void());
}

struct serialize_variant_visitor : public boost::static_visitor<std::string>
{
  template<class t_type>
  std::string operator()(const t_type& v) const
  {
    return epee::serialization::store_t_to_json(v);
  }
};

template <class t_variant>
std::string serialize_variant(const t_variant& v)
{
  TRY_ENTRY();
  return boost::apply_visitor(serialize_variant_visitor(), v);
  CATCH_ENTRY2("");
}


void MainWindow::on_core_event(const std::string event_name, const currency::core_event_v& e)
{
  TRY_ENTRY();
  //at the moment we don't forward CORE_EVENT_BLOCK_ADDEDevent to GUI
  if (CORE_EVENT_BLOCK_ADDED == event_name)
    return;

  m_events.m_que.push_back(currency::core_event());
  m_events.m_que.back().details = currency::core_event_v(e);
  m_events.m_que.back().method = event_name;
  CATCH_ENTRY2(void());
}

std::string get_events_que_json_string(const std::list<currency::core_event>& eq, std::string& methods_list)
{
  TRY_ENTRY();
  //t the moment portable_storage is not supporting polymorphic objects lists, so 
  //there is no hope to make serialization with variant list, lets handle it manual
  std::stringstream ss;
  ss << "{  \"events\" : [";
  uint64_t need_coma = false;
  for (const auto& e : eq)
  {
    if (need_coma)
    {
      ss << ",";
      methods_list += "|";
    }
    methods_list += e.method;
    ss << "{ \"method\": \"" << e.method << "\","  << ENDL;
    ss << "\"details\": " << serialize_variant(e.details) << ENDL << "}";
    need_coma = true;
  }
  ss << "]}";
  return ss.str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

void MainWindow::on_complete_events()
{
  TRY_ENTRY();
  if (m_events.m_que.size())
  {
    std::string methods_list;
    TIME_MEASURE_START_MS(core_events_handl_time);
    TIME_MEASURE_START_MS(json_buff_generate_time);
    std::string json_buff = get_events_que_json_string(m_events.m_que, methods_list);
    TIME_MEASURE_FINISH_MS(json_buff_generate_time);
    

    QMetaObject::invokeMethod(this, "on_core_event",
      Qt::QueuedConnection,
      Q_ARG(QString, QString(json_buff.c_str())));
    TIME_MEASURE_FINISH_MS(core_events_handl_time);
    LOG_PRINT_L0("SENT SIGNAL -> [CORE_EVENTS]: " << m_events.m_que.size() 
      << ", handle_time: " << core_events_handl_time << "(json: " << json_buff_generate_time << ")ms, json_buff size = " << json_buff.size() << ", methods: " << methods_list);
    LOG_PRINT_L2("CORE_EVENTS sent signal details: " << ENDL << json_buff);
    m_events.m_que.clear();

  }
  CATCH_ENTRY2(void());
}

void MainWindow::on_clear_events()
{
  TRY_ENTRY();
  m_events.m_que.clear();
  CATCH_ENTRY2(void());
}


QString MainWindow::store_secure_app_data(const QString& param, const QString& password)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  if (!tools::create_directories_if_necessary(m_backend.get_config_folder()))
  {
    view::api_response ar;
    LOG_PRINT_L0("Failed to create data directory: " << m_backend.get_config_folder());
    ar.error_code = API_RETURN_CODE_FAIL;
    return MAKE_RESPONSE(ar);
  }

  view::api_response ar = AUTO_VAL_INIT(ar);
  ar.error_code = tools::store_encrypted_file(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME,
    m_master_password, param.toStdString(), APP_DATA_FILE_BINARY_SIGNATURE);
  if (ar.error_code != API_RETURN_CODE_OK)
  {
    return MAKE_RESPONSE(ar);
  }

  crypto::hash master_password_pre_hash = crypto::cn_fast_hash(m_master_password.c_str(), m_master_password.length());
  crypto::hash master_password_hash = crypto::cn_fast_hash(&master_password_pre_hash, sizeof master_password_pre_hash);
  LOG_PRINT_L0("store_secure_app_data, r = " << ar.error_code << ", pass hash: " << master_password_hash);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_secure_app_data(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::password_data pwd = AUTO_VAL_INIT(pwd);

  if (!epee::serialization::load_t_from_json(pwd, param.toStdString()))
  {
    view::api_response ar;
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return MAKE_RESPONSE(ar);
  }

  std::string filename = m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME;
  std::string res_body;
  std::string rsp_code = tools::load_encrypted_file(filename, pwd.pass, res_body, APP_DATA_FILE_BINARY_SIGNATURE);    
  if (rsp_code != API_RETURN_CODE_OK)
  {
    view::api_response ar;
    ar.error_code = rsp_code;
    return MAKE_RESPONSE(ar);
  }
  m_master_password = pwd.pass;
  crypto::hash master_password_pre_hash = crypto::cn_fast_hash(m_master_password.c_str(), m_master_password.length());
  crypto::hash master_password_hash = crypto::cn_fast_hash(&master_password_pre_hash, sizeof master_password_pre_hash);
  LOG_PRINT_L0("gui secure config loaded ok from " << filename << ", pass hash: " << master_password_hash);

  return res_body.c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::set_master_password(const QString& param)
{
  view::api_response ar;

  view::password_data pwd = AUTO_VAL_INIT(pwd);

  if (!epee::serialization::load_t_from_json(pwd, param.toStdString()))
  {
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return MAKE_RESPONSE(ar);
  }

  if (!currency::validate_password(pwd.pass))
  {
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return MAKE_RESPONSE(ar);
  }

  m_master_password = pwd.pass;

  crypto::hash master_password_pre_hash = crypto::cn_fast_hash(m_master_password.c_str(), m_master_password.length());
  crypto::hash master_password_hash = crypto::cn_fast_hash(&master_password_pre_hash, sizeof master_password_pre_hash);
  LOG_PRINT_L0("set_master_password, pass hash: " << master_password_hash);

  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
}

QString MainWindow::check_master_password(const QString& param)
{
  view::password_data pwd = AUTO_VAL_INIT(pwd);
  view::api_response ar = AUTO_VAL_INIT(ar);

  if (!epee::serialization::load_t_from_json(pwd, param.toStdString()))
  {
    ar.error_code = API_RETURN_CODE_BAD_ARG;
    return MAKE_RESPONSE(ar);
  }

  crypto::hash master_password_pre_hash = crypto::cn_fast_hash(m_master_password.c_str(), m_master_password.length());
  crypto::hash master_password_hash = crypto::cn_fast_hash(&master_password_pre_hash, sizeof master_password_pre_hash);
  crypto::hash pwd_pre_hash = crypto::cn_fast_hash(pwd.pass.c_str(), pwd.pass.length());
  crypto::hash pwd_hash = crypto::cn_fast_hash(&pwd_pre_hash, sizeof pwd_pre_hash);
 
  if (m_master_password != pwd.pass)
  {
    ar.error_code = API_RETURN_CODE_WRONG_PASSWORD;
    LOG_PRINT_L0("check_master_password: pwd hash: " << pwd_hash << ", expected: " << master_password_hash);
  }
  else
  {
    ar.error_code = API_RETURN_CODE_OK;
  }
  return MAKE_RESPONSE(ar);
}

QString MainWindow::store_app_data(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::api_response ar;
  ar.error_code = API_RETURN_CODE_FAIL;
  if (!tools::create_directories_if_necessary(m_backend.get_config_folder()))
  {
    LOG_PRINT_L0("Failed to create data directory: " << m_backend.get_config_folder());
    return MAKE_RESPONSE(ar);
  }

  std::string filename = m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME;
  bool r = file_io_utils::save_string_to_file(filename, param.toStdString());
  if (r)
  {
    ar.error_code = API_RETURN_CODE_OK;
    LOG_PRINT_L1("config saved: " << filename);
  }
  else
  {
    ar.error_code = API_RETURN_CODE_FAIL;
    LOG_PRINT_L1("config save failed: " << filename);
  }

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::is_file_exist(const QString& path)
{
  TRY_ENTRY();
  try{
    bool r = file_io_utils::is_file_exist(path.toStdWString());
    if (r)
      return API_RETURN_CODE_ALREADY_EXISTS;
    else
      return API_RETURN_CODE_FILE_NOT_FOUND;
  }
  catch (const std::exception& ex)
  {
    LOG_ERROR("failed to check file existance: " << path.toStdString() << " ERROR:" << ex.what());
    return QString(API_RETURN_CODE_ALREADY_EXISTS) + ": " + ex.what();
  }

  catch (...)
  {
    return API_RETURN_CODE_ALREADY_EXISTS;
  }
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::store_to_file(const QString& path, const QString& buff)
{
  TRY_ENTRY();
  try{
    bool r = file_io_utils::save_string_to_file_throw(path.toStdWString(), buff.toStdString());
    if (r)
      return API_RETURN_CODE_OK;
    else
      return API_RETURN_CODE_ACCESS_DENIED;
  }
  catch (const std::exception& ex)
  {
    LOG_ERROR("FILED TO STORE TO FILE: " << path.toStdString() << " ERROR:" << ex.what());
    return QString(API_RETURN_CODE_ACCESS_DENIED) + ": " + ex.what();
  }

  catch (...)
  {
    return API_RETURN_CODE_ACCESS_DENIED;
  }

  
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::load_from_file(const QString& path)
{
  TRY_ENTRY();
  try {
    std::string buff;
    bool r = epee::file_io_utils::load_file_to_string(path.toStdWString(), buff);
    if (r)
      return QString::fromStdString(buff);
    else
      return QString();
  }
  catch (const std::exception& ex)
  {
    LOG_ERROR("FILED TO LOAD FROM FILE: " << path.toStdString() << " ERROR:" << ex.what());
    return QString(API_RETURN_CODE_ACCESS_DENIED) + ": " + ex.what();
  }

  catch (...)
  {
    return API_RETURN_CODE_ACCESS_DENIED;
  }


  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::get_app_data(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  std::string app_data_buff;
  file_io_utils::load_file_to_string(m_backend.get_config_folder() + "/" + GUI_CONFIG_FILENAME, app_data_buff);
  return app_data_buff.c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}


QString MainWindow::have_secure_app_data(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::api_response ar = AUTO_VAL_INIT(ar);

  boost::system::error_code ec;
  if (boost::filesystem::exists(epee::string_encoding::utf8_to_wstring(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME), ec))
    ar.error_code = API_RETURN_CODE_TRUE;
  else
    ar.error_code = API_RETURN_CODE_FALSE;

  LOG_PRINT_L0("have_secure_app_data, r = " << ar.error_code);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::drop_secure_app_data(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::api_response ar = AUTO_VAL_INIT(ar);

  boost::system::error_code ec;
  if (boost::filesystem::remove(epee::string_encoding::utf8_to_wstring(m_backend.get_config_folder() + "/" + GUI_SECURE_CONFIG_FILENAME), ec))
    ar.error_code = API_RETURN_CODE_TRUE;
  else
    ar.error_code = API_RETURN_CODE_FALSE;

  LOG_PRINT_L0("drop_secure_app_data, r = " << ar.error_code);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_all_aliases(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //PREPARE_ARG_FROM_JSON(currency::struct_with_one_t_type<uint64_t>, param);
  PREPARE_RESPONSE(view::alias_set, rsp);

  rsp.error_code = m_backend.get_aliases(rsp.response_data);
  QString res = MAKE_RESPONSE(rsp);
  LOG_PRINT_GREEN("GET_ALL_ALIASES: res: " <<  rsp.error_code << ", count: " << rsp.response_data.aliases.size() << ", string buff size: " << res.size(), LOG_LEVEL_1);
  return res;
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_alias_info_by_address(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(currency::alias_rpc_details, rsp);
  rsp.error_code = m_backend.get_alias_info_by_address(param.toStdString(), rsp.response_data);
  return MAKE_RESPONSE(rsp);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_alias_info_by_name(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(currency::alias_rpc_details, rsp);
  rsp.error_code = m_backend.get_alias_info_by_name(param.toStdString(), rsp.response_data);
  return MAKE_RESPONSE(rsp);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::validate_address(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::address_validation_response ar = AUTO_VAL_INIT(ar);
  ar.error_code = m_backend.validate_address(param.toStdString(), ar.payment_id);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::set_log_level(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(currency::struct_with_one_t_type<int64_t>, lvl);
  epee::log_space::get_set_log_detalisation_level(true, lvl.v);
  default_ar.error_code = API_RETURN_CODE_OK;
  LOG_PRINT("[LOG LEVEL]: set to " << lvl.v, LOG_LEVEL_MIN);
  
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_log_level(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_RESPONSE(currency::struct_with_one_t_type<int>, ar);
  ar.response_data.v = epee::log_space::get_set_log_detalisation_level();
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::set_enable_tor(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(currency::struct_with_one_t_type<bool>, enabl_tor);
  m_backend.set_use_tor(enabl_tor.v);
  //epee::log_space::get_set_log_detalisation_level(true, enabl_tor.v);
  default_ar.error_code = API_RETURN_CODE_OK;
  LOG_PRINT("[TOR]: Enable TOR set to " << enabl_tor.v, LOG_LEVEL_MIN);

  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

// QString MainWindow::dump_all_offers()
// {
//   LOG_API_TIMING();
//   //return que_call2<view::api_void>("dump_all_offers", "{}", [this](const view::api_void& owd, view::api_response& ar){
//   PREPARE_RESPONSE(view::api_void, ar);
//   //view::api_void av;
//   QString path = QFileDialog::getOpenFileName(this, "Select file",
//     "",
//     "");
// 
//   if (!path.length())
//   {
//     ar.error_code = API_RETURN_CODE_CANCELED;
//     return MAKE_RESPONSE(ar);
//   }
// 
//   currency::COMMAND_RPC_GET_OFFERS_EX::response rp = AUTO_VAL_INIT(rp);
//   ar.error_code = m_backend.get_all_offers(rp);
// 
//   std::string buff = epee::serialization::store_t_to_json(rp);
//   bool r = file_io_utils::save_string_to_file(path.toStdString(), buff);
//   if (!r)
//     ar.error_code = API_RETURN_CODE_FAIL;
//   else
//     ar.error_code = API_RETURN_CODE_OK;
// 
//   return MAKE_RESPONSE(ar);
// }

QString MainWindow::webkit_launched_script(const QString& param)
{
  TRY_ENTRY();
  m_last_update_daemon_status_json.clear();
  return "";
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}
////////////////////
QString MainWindow::show_openfile_dialog(const QString& param)
{
  TRY_ENTRY();
  view::system_filedialog_request ofdr = AUTO_VAL_INIT(ofdr);
  view::system_filedialog_response ofdres = AUTO_VAL_INIT(ofdres);
  if (!epee::serialization::load_t_from_json(ofdr, param.toStdString()))
  {
    ofdres.error_code = API_RETURN_CODE_BAD_ARG;
    return epee::serialization::store_t_to_json(ofdres, 0).c_str();
  }

  QString path = QFileDialog::getOpenFileName(this, ofdr.caption.c_str(),
    ofdr.default_dir.c_str(),
    ofdr.filemask.c_str());

  if (!path.length())
  {
    ofdres.error_code = API_RETURN_CODE_CANCELED;
    return epee::serialization::store_t_to_json(ofdres, 0).c_str();
  }

  ofdres.error_code = API_RETURN_CODE_OK;
  ofdres.path = path.toStdString();
  return MAKE_RESPONSE(ofdres); 
  CATCH_ENTRY_FAIL_API_RESPONCE();
}


QString MainWindow::show_savefile_dialog(const QString& param)
{
  TRY_ENTRY();
  PREPARE_ARG_FROM_JSON(view::system_filedialog_request, ofdr);
  view::system_filedialog_response ofdres = AUTO_VAL_INIT(ofdres);

  QString path = QFileDialog::getSaveFileName(this, ofdr.caption.c_str(),
    ofdr.default_dir.c_str(),
    ofdr.filemask.c_str());

  if (!path.length())
  {
    ofdres.error_code = API_RETURN_CODE_CANCELED;
    return epee::serialization::store_t_to_json(ofdres, 0).c_str();
  }

  ofdres.error_code = API_RETURN_CODE_OK;
  ofdres.path = path.toStdString();
  return MAKE_RESPONSE(ofdres);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::close_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::wallet_id_obj>("close_wallet", param, [this](const view::wallet_id_obj& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, owd);
  PREPARE_RESPONSE(view::api_void, ar);
  ar.error_code = m_backend.close_wallet(owd.wallet_id);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_contracts(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, owd);
  PREPARE_RESPONSE(tools::wallet_public::contracts_array, ar);
  ar.error_code = m_backend.get_contracts(owd.wallet_id, ar.response_data.contracts);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::create_proposal(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::create_proposal_param_gui, cpp);
  PREPARE_RESPONSE(tools::wallet_public::contracts_array, ar);
  ar.error_code = m_backend.create_proposal(cpp);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}


QString MainWindow::accept_proposal(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_and_contract_id_param, waip);
  PREPARE_RESPONSE(view::api_void, ar);

  ar.error_code = m_backend.accept_proposal(waip.wallet_id, waip.contract_id);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::release_contract(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::release_contract_param, rcp);
  PREPARE_RESPONSE(view::api_void, ar);

  ar.error_code = m_backend.release_contract(rcp.wallet_id, rcp.contract_id, rcp.release_type);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::request_cancel_contract(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::crequest_cancel_contract_param, rcp);
  PREPARE_RESPONSE(view::api_void, ar);

  ar.error_code = m_backend.request_cancel_contract(rcp.wallet_id, rcp.contract_id, rcp.fee, rcp.expiration_period);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::accept_cancel_contract(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_and_contract_id_param, wci);
  PREPARE_RESPONSE(view::api_void, ar);

  ar.error_code = m_backend.accept_cancel_contract(wci.wallet_id, wci.contract_id);

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::generate_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::open_wallet_request>("generate_wallet", param, [this](const view::open_wallet_request& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::open_wallet_request, owd);
  PREPARE_RESPONSE(view::open_wallet_response, ar);
  ar.error_code = m_backend.generate_wallet(epee::string_encoding::utf8_to_wstring(owd.path), owd.pass, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}


QString MainWindow::restore_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::restore_wallet_request>("restore_wallet", param, [this](const view::restore_wallet_request& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::restore_wallet_request, owd);
  PREPARE_RESPONSE(view::open_wallet_response, ar);
  ar.error_code = m_backend.restore_wallet(epee::string_encoding::utf8_to_wstring(owd.path), owd.pass, owd.seed_phrase, owd.seed_pass, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::use_whitelisting(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::restore_wallet_request>("restore_wallet", param, [this](const view::restore_wallet_request& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::api_request_t<bool>, owd);
  PREPARE_RESPONSE(view::api_responce_return_code, ar);
  ar.error_code = m_backend.use_whitelisting(owd.wallet_id, owd.req_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::open_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();

  //return que_call2<view::open_wallet_request>("open_wallet", param, [this](const view::open_wallet_request& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::open_wallet_request, owd);
  PREPARE_RESPONSE(view::open_wallet_response, ar);
  ar.error_code = m_backend.open_wallet(epee::string_encoding::utf8_to_wstring(owd.path), owd.pass, owd.txs_to_return, ar.response_data, owd.exclude_mining_txs);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_my_offers(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::open_wallet_request>("open_wallet", param, [this](const view::open_wallet_request& owd, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(bc_services::core_offers_filter, f);
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_OFFERS_EX::response, ar);
  ar.error_code = m_backend.get_my_offers(f, ar.response_data.offers);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_fav_offers(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::get_fav_offers_request, f);
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_OFFERS_EX::response, ar);
  ar.error_code = m_backend.get_fav_offers(f.ids, f.filter, ar.response_data.offers);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::is_pos_allowed(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  return m_backend.is_pos_allowed().c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::run_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wio);

  default_ar.error_code = m_backend.run_wallet(wio.wallet_id);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::resync_wallet(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<view::wallet_id_obj>("get_wallet_info", param, [this](const view::wallet_id_obj& a, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, a);
  PREPARE_RESPONSE(view::api_void, ar);

  ar.error_code = m_backend.resync_wallet(a.wallet_id);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_offers_ex(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return que_call2<bc_services::core_offers_filter>("get_offers_ex", param, [this](const bc_services::core_offers_filter& f, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(bc_services::core_offers_filter, f);
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_OFFERS_EX::response, ar);
  ar.error_code = m_backend.get_offers_ex(f, ar.response_data.offers, ar.response_data.total_offers);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::push_offer(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  LOG_API_PARAMS(LOG_LEVEL_2);
  //return que_call2<view::push_offer_param>("push_offer", param, [this](const view::push_offer_param& a, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::push_offer_param, a);
  PREPARE_RESPONSE(view::transfer_response, ar);

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

  ar.error_code = m_backend.push_offer(a.wallet_id, a.od, res_tx);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);
  
  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::cancel_offer(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  LOG_API_PARAMS(LOG_LEVEL_2);
  //  return que_call2<view::cancel_offer_param>("cancel_offer", param, [this](const view::cancel_offer_param& a, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(view::cancel_offer_param, a);
  PREPARE_RESPONSE(view::transfer_response, ar);

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

  ar.error_code = m_backend.cancel_offer(a, res_tx);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);

  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::push_update_offer(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  LOG_API_PARAMS(LOG_LEVEL_2);
  //return que_call2<bc_services::update_offer_details>("cancel_offer", param, [this](const bc_services::update_offer_details& a, view::api_response& ar){
  PREPARE_ARG_FROM_JSON(bc_services::update_offer_details, a);
  PREPARE_RESPONSE(view::transfer_response, ar);

  currency::transaction res_tx = AUTO_VAL_INIT(res_tx);

  ar.error_code = m_backend.push_update_offer(a, res_tx);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);

  ar.response_data.success = true;
  ar.response_data.tx_hash = string_tools::pod_to_hex(currency::get_transaction_hash(res_tx));
  ar.response_data.tx_blob_size = currency::get_object_blobsize(res_tx);
  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_recent_transfers(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::get_recent_transfers_request, a);
  PREPARE_RESPONSE(view::transfers_array, ar);
  ar.error_code = m_backend.get_recent_transfers(a.wallet_id, a.offset, a.count, ar.response_data, a.exclude_mining_txs);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}


QString MainWindow::get_mining_history(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  //return prepare_call<view::wallet_id_obj, tools::wallet_rpc::mining_history>("get_mining_history", param, [this](const view::wallet_id_obj& a, view::api_response& ar) {
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, a);
  PREPARE_RESPONSE(tools::wallet_public::mining_history, ar);

  ar.error_code = m_backend.get_mining_history(a.wallet_id, ar.response_data);
  if (ar.error_code != API_RETURN_CODE_OK)
    return MAKE_RESPONSE(ar);

  ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::start_pos_mining(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wo);
  default_ar.error_code = m_backend.start_pos_mining(wo.wallet_id);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::stop_pos_mining(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, wo);
  default_ar.error_code = m_backend.stop_pos_mining(wo.wallet_id);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_smart_wallet_info(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::request_get_smart_wallet_info, wo);
  PREPARE_RESPONSE(view::get_restore_info_response, ar);
  ar.error_code = m_backend.get_wallet_restore_info(wo.wallet_id, ar.response_data.seed_phrase, wo.seed_password);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_mining_estimate(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::request_mining_estimate, me);
  PREPARE_RESPONSE(view::response_mining_estimate, ar);
  ar.error_code = m_backend.get_mining_estimate(me.amount_coins, me.time, ar.response_data.final_amount, ar.response_data.all_coins_and_pos_diff_rate, ar.response_data.days_estimate);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::add_custom_asset_id(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_and_asset_id, waid);
  PREPARE_RESPONSE(currency::COMMAND_RPC_GET_ASSET_INFO::response, ar);

  ar.error_code = m_backend.add_custom_asset_id(waid.wallet_id, waid.asset_id, ar.response_data.asset_descriptor);
  ar.response_data.status = ar.error_code;
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::remove_custom_asset_id(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_and_asset_id, waid);
  default_ar.error_code = m_backend.delete_custom_asset_id(waid.wallet_id, waid.asset_id);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::get_wallet_info(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::wallet_id_obj, waid);
  PREPARE_RESPONSE(view::wallet_info, ar);
  ar.error_code = m_backend.get_wallet_info(waid.wallet_id, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::create_ionic_swap_proposal(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::create_ionic_swap_proposal_request, cispr);
  PREPARE_RESPONSE(std::string, ar);
  ar.error_code = m_backend.create_ionic_swap_proposal(cispr.wallet_id, cispr, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::get_ionic_swap_proposal_info(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::api_request_t<std::string>, tx_raw_hex);
  PREPARE_RESPONSE(tools::wallet_public::ionic_swap_proposal_info, ar);
  ar.error_code = m_backend.get_ionic_swap_proposal_info(tx_raw_hex.wallet_id, tx_raw_hex.req_data, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();

}

QString MainWindow::accept_ionic_swap_proposal(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::api_request_t<std::string>, tx_raw_hex);
  PREPARE_RESPONSE(std::string, ar);
  ar.error_code = m_backend.accept_ionic_swap_proposal(tx_raw_hex.wallet_id, tx_raw_hex.req_data, ar.response_data);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::backup_wallet_keys(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::backup_keys_request, me);
  default_ar.error_code = m_backend.backup_wallet(me.wallet_id, epee::string_encoding::utf8_to_wstring(me.path));
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::reset_wallet_password(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::reset_pass_request, me);
  default_ar.error_code = m_backend.reset_wallet_password(me.wallet_id, me.pass);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
QString MainWindow::is_wallet_password_valid(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::reset_pass_request, me);
  default_ar.error_code = m_backend.is_wallet_password_valid(me.wallet_id, me.pass);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::is_autostart_enabled(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  view::api_response ar;

  if (gui_tools::GetStartOnSystemStartup())
    ar.error_code = API_RETURN_CODE_TRUE;
  else
    ar.error_code = API_RETURN_CODE_FALSE;

  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::toggle_autostart(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(currency::struct_with_one_t_type<bool>, as);

  if (gui_tools::SetStartOnSystemStartup(as.v))
    default_ar.error_code = API_RETURN_CODE_OK;
  else
    default_ar.error_code = API_RETURN_CODE_FAIL;

  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}
/*
QString MainWindow::check_available_sources(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::api_request_t<std::list<uint64_t> >, sources);
  return m_backend.check_available_sources(sources.wallet_id, sources.req_data).c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}
*/

QString MainWindow::open_url_in_browser(const QString& param)
{
  TRY_ENTRY();
  QString prefix = "https://";
  if (!QDesktopServices::openUrl(QUrl(prefix + param)))
  {
    LOG_ERROR("Failed top open URL: " << param.toStdString());
    return API_RETURN_CODE_FAIL;
  }
  LOG_PRINT_L0("[Open URL]: " << param.toStdString());
  return API_RETURN_CODE_OK;
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::setup_jwt_wallet_rpc(const QString& param)
{
  TRY_ENTRY();

  m_backend.setup_wallet_rpc(param.toStdString());

  return API_RETURN_CODE_OK;
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::is_valid_restore_wallet_text(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::seed_info_param, rwtp);
  return m_backend.is_valid_brain_restore_data(rwtp.seed_phrase, rwtp.seed_password).c_str();
  CATCH_ENTRY2(API_RETURN_CODE_INTERNAL_ERROR);
}

QString MainWindow::get_seed_phrase_info(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::seed_info_param, rwtp);
  PREPARE_RESPONSE(view::seed_phrase_info, ar);
  ar.error_code = m_backend.get_seed_phrase_info(rwtp.seed_phrase, rwtp.seed_password, ar.response_data).c_str();
  LOG_PRINT_CYAN("[get_seed_phrase_info]:" << epee::serialization::store_t_to_json(ar), LOG_LEVEL_0);
  return MAKE_RESPONSE(ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

void MainWindow::contextMenuEvent(QContextMenuEvent * event)
{
  TRY_ENTRY();

  CATCH_ENTRY2(void());
}
QString MainWindow::print_text(const QString& param)
{
  TRY_ENTRY();
  LOG_API_TIMING();
  PREPARE_ARG_FROM_JSON(view::print_text_param, ptp);

  //in >> htmlContent;

  QTextDocument *document = new QTextDocument();
  document->setHtml(ptp.html_text.c_str());

  QPrinter printer;
  default_ar.error_code = API_RETURN_CODE_CANCELED;

  QPrintDialog *dialog = new QPrintDialog(&printer, this);
  dialog->setOptions(QAbstractPrintDialog::PrintToFile);
  auto res = dialog->exec();
  if (res != QDialog::Accepted)
  {
    LOG_PRINT_L0("[PRINT_TEXT] exec  != QDialog::Accepted, res=" << res);
    return MAKE_RESPONSE(default_ar);
  }

  document->print(&printer);

  delete document;
  default_ar.error_code = API_RETURN_CODE_OK;
  LOG_PRINT_L0("[PRINT_TEXT] default_ar.error_code = " << default_ar.error_code);
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

QString MainWindow::print_log(const QString& param)
{
  TRY_ENTRY();
  PREPARE_ARG_FROM_JSON(view::print_log_params, plp);

  LOG_PRINT("[GUI_LOG]" << plp.msg, plp.log_level);

  default_ar.error_code = API_RETURN_CODE_OK;
  return MAKE_RESPONSE(default_ar);
  CATCH_ENTRY_FAIL_API_RESPONCE();
}

void MainWindow::show_notification(const std::string& title, const std::string& message)
{
  TRY_ENTRY();
  LOG_PRINT_L1("system notification: \"" << title << "\", \"" << message << "\"");
  
  // it's expected that title and message are utf-8 encoded!
  
#if !defined(__APPLE__)
  // use Qt tray icon to show messages on Windows and Linux
  CHECK_AND_ASSERT_MES(m_tray_icon != nullptr, (void)(0), "m_tray_icon is null!");
  m_tray_icon->showMessage(QString().fromUtf8(title.c_str()), QString().fromUtf8(message.c_str()));
#else
  // use native notification system on macOS
  notification_helper::show(title, message);
#endif
  CATCH_ENTRY2(void());
}


