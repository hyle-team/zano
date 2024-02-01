// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Boolberry developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include "application/mainwindow.h"
#include "qdebug.h"
#include <thread>
#include <math.h>
//#include "qtlogger.h"
#include "include_base_utils.h"
#include "currency_core/currency_config.h"
#ifdef Q_OS_DARWIN
#include "application/urleventfilter.h"
#endif

int main(int argc, char *argv[])
{
//#if defined(ARCH_CPU_X86_64) && _MSC_VER <= 1800
       // VS2013's CRT only checks the existence of FMA3 instructions, not the
       // enabled-ness of them at the OS level (this is fixed in VS2015). We force
       // off usage of FMA3 instructions in the CRT to avoid using that path and
       // hitting illegal instructions when running on CPUs that support FMA3, but
       // OSs that don't. Because we use the static library CRT we have to call
       // this function once in each DLL.
       // See http://crbug.com/436603.
//       _set_FMA3_enable(0);
//#endif  // ARCH_CPU_X86_64 && _MSC_VER <= 1800
  
  if(argc > 1)
    std::cout << argv[1] << std::endl;

#ifdef _MSC_VER 
  #ifdef _WIN64
  _set_FMA3_enable(0);
  #endif
  //mutex to let InnoSetup know about running instance
  ::CreateMutex(NULL, FALSE, CURRENCY_NAME_BASE "_instance");
  //::CreateMutex(NULL, FALSE, "Global\\" CURRENCY_NAME_BASE "_instance");
#endif


#ifdef WIN32
  WCHAR sz_file_name[MAX_PATH + 1] = L"";
  ::GetModuleFileNameW(NULL, sz_file_name, MAX_PATH + 1);
  std::string path_to_process_utf8 = epee::string_encoding::wstring_to_utf8(sz_file_name);
#else
  std::string path_to_process_utf8 = argv[0];
#endif

  TRY_ENTRY();
  epee::string_tools::set_module_name_and_folder(path_to_process_utf8);
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#ifdef _MSC_VER
#if _MSC_VER >= 1910
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps); //HiDPI pixmaps
  //qputenv("QT_SCALE_FACTOR", "0.75");
#endif
#endif

  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_0);
  log_space::get_set_need_thread_id(true, true);
  log_space::log_singletone::enable_channels("core,currency_protocol,tx_pool,p2p,wallet");


  QApplication app(argc, argv);

  MainWindow viewer;
  if (!viewer.init_backend(argc, argv))
  {
    return 1;
  }

#ifdef Q_OS_DARWIN
  URLEventFilter url_event_filter(&viewer);
  app.installEventFilter(&url_event_filter);
#endif

  app.installNativeEventFilter(&viewer);
  viewer.setWindowTitle(CURRENCY_NAME_BASE);
  viewer.show_inital();

  int res = app.exec();
  LOG_PRINT_L0("Process exit with code: " << res);
  return res;
  CATCH_ENTRY2(0);
}
