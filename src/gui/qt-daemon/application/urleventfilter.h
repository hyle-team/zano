#include <QObject>
#include <QFileOpenEvent>
#include <QDebug>
#include "mainwindow.h"

class URLEventFilter : public QObject
{
  Q_OBJECT
public:
  URLEventFilter(MainWindow* pmainwindow) : m_pmainwindow(pmainwindow),QObject()
  {};
protected:
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  MainWindow* m_pmainwindow;
};