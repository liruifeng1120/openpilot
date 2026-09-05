#pragma once

#include <QStackedLayout>
#include <QWidget>

#include "selfdrive/ui/qt/home.h"
#include "selfdrive/ui/qt/offroad/onboarding.h"
#include "selfdrive/ui/qt/offroad/settings.h"

class MainWindow : public QWidget {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);

private:
  bool eventFilter(QObject *obj, QEvent *event) override;
  void openSettings(int index = 0, const QString &param = "");
  void closeSettings();
  void updateState(const UIState &s);
  void doPoweroff();

  QStackedLayout *main_layout;
  HomeWindow *homeWindow;
  SettingsWindow *settingsWindow;
  OnboardingWindow *onboardingWindow;

  // 两次连续挂 P 档触发关机
  qint64 last_p_gear_ts = 0;       // 上次进入 P 档的时间戳（ms）
  bool was_p_gear = false;         // 上一帧档位是否为 P
  static constexpr int P_GEAR_DOUBLE_TAP_MS = 3000;  // 双击 P 档时间窗口
};
