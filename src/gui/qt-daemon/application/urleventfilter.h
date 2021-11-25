#include <QObject>
#include <QFileOpenEvent>
#include <QDebug>

class URLEventFilter : public QObject
{
  Q_OBJECT
public:
  URLEventFilter() : QObject(){};
protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
};