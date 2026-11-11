// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <sstream>

//#define QT_CONSOLE_STREAM qDebug()

#define QT_CONSOLE_STREAM(s) \
{std::stringstream ss; ss << s; fputs(ss.str().c_str(), stderr); fflush(stderr);}



//fputs(buf.c_str(), stderr); 
//fflush(stderr);


void customHandler(QtMsgType type, const char* msg) {
  fputs(msg, stderr);
  fflush(stderr);
}


class qt_console_stream : public epee::log_space::ibase_log_stream
{
  QDebug db;
public:
  qt_console_stream() : db(qDebug())
  {
    // Somewhere in your program
    //qInstallMsgHandler(customHandler);
  }

  ~qt_console_stream()
  {
  }

  int get_type(){ return LOGGER_CONSOLE; }

  inline void set_console_color(int color, bool bright)
  {
    switch (color)
    {
    case epee::log_space::console_color_default:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;37m");}
      else
        {QT_CONSOLE_STREAM("\033[0m");}
      break;
    case epee::log_space::console_color_white:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;37m");}
      else
        {QT_CONSOLE_STREAM("\033[0;37m");}
      break;
    case epee::log_space::console_color_red:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;31m");}
      else
        {QT_CONSOLE_STREAM("\033[0;31m");}
      break;
    case epee::log_space::console_color_green:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;32m");}
      else
        {QT_CONSOLE_STREAM("\033[0;32m");}
      break;

    case epee::log_space::console_color_blue:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;34m");}
      else
        {QT_CONSOLE_STREAM("\033[0;34m");}
      break;

    case epee::log_space::console_color_cyan:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;36m");}
      else
        {QT_CONSOLE_STREAM("\033[0;36m");}
      break;

    case epee::log_space::console_color_magenta:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;35m");}
      else
        {QT_CONSOLE_STREAM("\033[0;35m");}
      break;

    case epee::log_space::console_color_yellow:
      if (bright)
        {QT_CONSOLE_STREAM("\033[1;33m");}
      else
        {QT_CONSOLE_STREAM("\033[0;33m");}
      break;

    }
  }

  inline void reset_console_color() 
  {
    {QT_CONSOLE_STREAM("\033[0m");}
  }

  virtual bool out_buffer(const char* buffer, int buffer_len, int log_level, int color, const char* plog_name = NULL)
  {
    if (plog_name)
      return true; //skip alternative logs from console

    set_console_color(color, log_level < 1);

    std::string buf(buffer, buffer_len);
    for (size_t i = 0; i != buf.size(); i++)
    {
      if (buf[i] == 7 || buf[i] == -107)
        buf[i] = '^';
      //remove \n
      //if (i == buf.size()-1)
      //  buf[i] = ' ';
    }

    QT_CONSOLE_STREAM(buf.c_str());

   
    reset_console_color();
    return  true;
  }


};
