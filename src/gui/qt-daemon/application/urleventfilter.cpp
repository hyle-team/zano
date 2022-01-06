#include "urleventfilter.h"
#include <QMessageBox>

bool URLEventFilter::eventFilter(QObject *obj, QEvent *event)
{
      if (event->type() == QEvent::FileOpen) {
          QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
          if(!fileEvent->url().isEmpty())
          {
            m_pmainwindow->handle_deeplink_click(fileEvent->url().toString());
            //QMessageBox msg;
            //msg.setText(fileEvent->url().toString());
            //msg.exec();
            return true;
          }
          return true;
      } else {
          // standard event processing
          return QObject::eventFilter(obj, event);
      }
  };
