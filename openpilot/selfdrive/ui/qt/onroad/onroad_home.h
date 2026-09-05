#pragma once

#include "selfdrive/ui/qt/onroad/alerts.h"
#include "selfdrive/ui/qt/onroad/annotated_camera.h"
#include "selfdrive/ui/qt/scrcpy/scrcpy_widget.h"

#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class CameraWidget;

class OnroadWindow : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  OnroadWindow(QWidget* parent = 0);
  bool isMapVisible() const { return map && map->isVisible(); }
  void showMapPanel(bool show) { if (map) map->setVisible(show); }

signals:
  void mapPanelRequested();

private:
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent* e) override;
  void mouseDoubleClickEvent(QMouseEvent* e) override;
  void resizeEvent(QResizeEvent *event) override;
  OnroadAlerts *alerts;
  AnnotatedCameraWidget *nvg;
  QColor bg = bg_colors[STATUS_DISENGAGED];
  QColor bg_long = bg_colors[STATUS_DISENGAGED];
  QWidget *map = nullptr;
  QHBoxLayout* split;
  // ���Ӻ��Ӿ����棺ͨ�� VisionIPC ���� cam_blindspot ���͵�����ä������
  // ����ת�����ʾ��ä��������ת�����ʾ��ä��
  CameraWidget *blind_left = nullptr;
  CameraWidget *blind_right = nullptr;
  // �ֻ�Ͷ�� (scrcpy)���� UI ����ʾ�ֻ����沢֧�ַ���������
  ScrcpyWidget *phone_mirror = nullptr;
  bool phone_mirror_on_ = false;
  bool phone_mirror_placed_ = false;
  bool phone_mirror_user_hidden_ = false;
  bool phone_mirror_auto_shown_ = false;
  bool phone_mirror_maximized_ = false;

private slots:
  void offroadTransition(bool offroad);
  void updateState(const UIState &s);

protected:
    void initializeGL() override;
};
