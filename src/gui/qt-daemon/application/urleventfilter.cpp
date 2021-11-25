#include "urleventfilter.h"
#include <QMessageBox>

bool URLEventFilter::eventFilter(QObject *obj, QEvent *event)
{
      if (event->type() == QEvent::FileOpen) {
          QFileOpenEvent *fileEvent = static_cast<QFileOpenEvent*>(event);
          if(!fileEvent->url().isEmpty())
          {
            QMessageBox msg;
            msg.setText(fileEvent->url().toString());
            msg.exec();
          }
      } else {
          // standard event processing
          return QObject::eventFilter(obj, event);
      }
  };
