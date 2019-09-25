// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2015 Boolberry developers
// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gui_utils.h"

// #include <QAbstractItemView>
#include <QApplication>
// #include <QClipboard>
// #include <QDateTime>
// #include <QDesktopServices>
// #include <QDesktopWidget>
// #include <QDoubleValidator>
// #include <QFileDialog>
// #include <QFont>
// #include <QLineEdit>
#include <QSettings>
// #include <QTextDocument> // for Qt::mightBeRichText
// #include <QThread>

// #if QT_VERSION < 0x050000
// #include <QUrl>
// #else
// #include <QUrlQuery>
// #endif
#include <stdint.h>
#include <math.h>
#include "currency_core/currency_config.h"
#define GUI_LINK_NAME "Zano"

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#if BOOST_FILESYSTEM_VERSION >= 3
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>
#endif
#include <boost/scoped_array.hpp>

#ifndef MAX_PATH
#define MAX_PATH 1000
#endif

#if BOOST_FILESYSTEM_VERSION >= 3
static boost::filesystem::detail::utf8_codecvt_facet utf8;
#endif



namespace gui_tools
{

#ifdef WIN32
  boost::filesystem::path GetSpecialFolderPath(int nFolder, bool fCreate = true)
  {
    namespace fs = boost::filesystem;

    wchar_t pszPath[MAX_PATH] = L"";

    if (SHGetSpecialFolderPathW(NULL, pszPath, nFolder, fCreate))
    {
      return fs::path(pszPath);
    }

    //LogPrintf("SHGetSpecialFolderPathW() failed, could not obtain requested path.\n");
    return fs::path("");
  }
#endif
#ifdef WIN32
  boost::filesystem::path static StartupShortcutPath()
  {
    return GetSpecialFolderPath(CSIDL_STARTUP) / GUI_LINK_NAME ".lnk";
  }

  bool GetStartOnSystemStartup()
  {
    // check for Bitcoin*.lnk
    return boost::filesystem::exists(StartupShortcutPath());
  }

  bool SetStartOnSystemStartup(bool fAutoStart)
  {
    // If the shortcut exists already, remove it for updating
    boost::system::error_code ec;
    std::wstring shrtcut_path = StartupShortcutPath().c_str();
    boost::filesystem::remove(shrtcut_path.c_str(), ec);
    if (ec)
    {
      //LOG_ERROR("Autostart disable failed: " << ec.message());
    }

    if (fAutoStart)
    {
      CoInitialize(NULL);

      // Get a pointer to the IShellLink interface.
      IShellLink* psl = NULL;
      HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink,
        reinterpret_cast<void**>(&psl));

      if (SUCCEEDED(hres))
      {
        // Get the current executable path
        TCHAR pszExePath[MAX_PATH];
        GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

        // Start client minimized
        //QString strArgs = "-min";
        // Set -testnet /-regtest options
        //strArgs += QString::fromStdString(strprintf(" -testnet=%d -regtest=%d", GetBoolArg("-testnet", false), GetBoolArg("-regtest", false)));

//#ifdef UNICODE
//        boost::scoped_array<TCHAR> args(new TCHAR[strArgs.length() + 1]);
        // Convert the QString to TCHAR*
//        strArgs.toWCharArray(args.get());
        // Add missing '\0'-termination to string
//        args[strArgs.length()] = '\0';
//#endif

        // Set the path to the shortcut target
        psl->SetPath(pszExePath);
        PathRemoveFileSpec(pszExePath);
        psl->SetWorkingDirectory(pszExePath);
        psl->SetShowCmd(SW_SHOWMINNOACTIVE);
//#ifndef UNICODE
//        psl->SetArguments(strArgs.toStdString().c_str());
//#else
//        psl->SetArguments(args.get());
//#endif

        // Query IShellLink for the IPersistFile interface for
        // saving the shortcut in persistent storage.
        IPersistFile* ppf = NULL;
        hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
        if (SUCCEEDED(hres))
        {
          WCHAR pwsz[MAX_PATH];
          // Ensure that the string is ANSI.
          MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().string().c_str(), -1, pwsz, MAX_PATH);
          // Save the link by calling IPersistFile::Save.
          hres = ppf->Save(pwsz, TRUE);
          ppf->Release();
          psl->Release();
          CoUninitialize();
          return true;
        }
        psl->Release();
      }
      CoUninitialize();
      return false;
    }
    return true;
  }
#elif defined(Q_OS_LINUX)

  // Follow the Desktop Application Autostart Spec:
  // http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

  boost::filesystem::path static GetAutostartDir()
  {
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
  }

  boost::filesystem::path static GetAutostartFilePath()
  {
    return GetAutostartDir() / GUI_LINK_NAME ".desktop";
  }

  bool GetStartOnSystemStartup()
  {
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
      return false;
    // Scan through file for "Hidden=true":
    std::string line;
    while (!optionFile.eof())
    {
      getline(optionFile, line);
      if (line.find("Hidden") != std::string::npos &&
        line.find("true") != std::string::npos)
        return false;
    }
    optionFile.close();

    return true;
  }

  bool SetStartOnSystemStartup(bool fAutoStart)
  {
    if (!fAutoStart)
      boost::filesystem::remove(GetAutostartFilePath());
    else
    {
      char pszExePath[MAX_PATH + 1];
      memset(pszExePath, 0, sizeof(pszExePath));
      if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
        return false;

      boost::filesystem::create_directories(GetAutostartDir());

      boost::filesystem::ofstream optionFile(GetAutostartFilePath(), std::ios_base::out | std::ios_base::trunc);
      if (!optionFile.good())
        return false;
      // Write a bitcoin.desktop file to the autostart directory:
      optionFile << "[Desktop Entry]\n";
      optionFile << "Type=Application\n";
      optionFile << "Name=" << CURRENCY_NAME_BASE << "\n";
      optionFile << "Exec=" << pszExePath << "\n";
      optionFile << "Terminal=false\n";
      optionFile << "Hidden=false\n";
      optionFile.close();
    }
    return true;
  }


#elif defined(Q_OS_MAC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // based on: https://github.com/Mozketo/LaunchAtLoginController/blob/master/LaunchAtLoginController.m
    
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
    
  LSSharedFileListItemRef findStartupItemInList(LSSharedFileListRef list, CFURLRef findUrl);
  LSSharedFileListItemRef findStartupItemInList(LSSharedFileListRef list, CFURLRef findUrl)
  {
    SInt32 macos_ver_maj, macos_ver_min;
    Gestalt(gestaltSystemVersionMajor, &macos_ver_maj); // very old and deprecated, consider change
    Gestalt(gestaltSystemVersionMinor, &macos_ver_min); // very old and deprecated, consider change
    
    // loop through the list of startup items and try to find the bitcoin app
    CFArrayRef listSnapshot = LSSharedFileListCopySnapshot(list, NULL);
    for(int i = 0; i < CFArrayGetCount(listSnapshot); i++) {
      LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(listSnapshot, i);
      UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
      CFURLRef currentItemURL = NULL;
      
#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED >= 10100
      if (&LSSharedFileListItemCopyResolvedURL != NULL && (macos_ver_maj > 10 || (macos_ver_maj == 10 && macos_ver_min >= 10)))
        currentItemURL = LSSharedFileListItemCopyResolvedURL(item, resolutionFlags, NULL); // this is available only on macOS 10.10-10.11 (then deprecated)
#if defined(MAC_OS_X_VERSION_MIN_REQUIRED) && MAC_OS_X_VERSION_MIN_REQUIRED < 10100
      else
        LSSharedFileListItemResolve(item, resolutionFlags, &currentItemURL, NULL);
#endif
#else
      LSSharedFileListItemResolve(item, resolutionFlags, &currentItemURL, NULL);
#endif
      
      if(currentItemURL && CFEqual(currentItemURL, findUrl)) {
        // found
        CFRelease(currentItemURL);
        return item;
      }
      if(currentItemURL) {
        CFRelease(currentItemURL);
      }
    }
    return NULL;
  }
  
    bool GetStartOnSystemStartup()
    {
        CFURLRef bitcoinAppUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
        LSSharedFileListItemRef foundItem = findStartupItemInList(loginItems, bitcoinAppUrl);
        return !!foundItem; // return boolified object
    }
    
    bool SetStartOnSystemStartup(bool fAutoStart)
    {
        CFURLRef bitcoinAppUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
        LSSharedFileListItemRef foundItem = findStartupItemInList(loginItems, bitcoinAppUrl);
        
        if(fAutoStart && !foundItem) {
            // add bitcoin app to startup item list
            LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemBeforeFirst, NULL, NULL, bitcoinAppUrl, NULL, NULL);
        }
        else if(!fAutoStart && foundItem) {
            // remove item
            LSSharedFileListItemRemove(loginItems, foundItem);
        }
        return true;
    }
#pragma GCC diagnostic pop
#else

  bool GetStartOnSystemStartup() { return false; }
  bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif
}
