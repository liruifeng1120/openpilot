#include "selfdrive/ui/qt/window.h"

#include <QFontDatabase>
#include <QLabel>

#include "system/hardware/hw.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  main_layout = new QStackedLayout(this);
  main_layout->setMargin(0);

  homeWindow = new HomeWindow(this);
  main_layout->addWidget(homeWindow);
  QObject::connect(homeWindow, &HomeWindow::openSettings, this, &MainWindow::openSettings);
  QObject::connect(homeWindow, &HomeWindow::closeSettings, this, &MainWindow::closeSettings);

  settingsWindow = new SettingsWindow(this);
  main_layout->addWidget(settingsWindow);
  QObject::connect(settingsWindow, &SettingsWindow::closeSettings, this, &MainWindow::closeSettings);
  QObject::connect(settingsWindow, &SettingsWindow::reviewTrainingGuide, [=]() {
    onboardingWindow->showTrainingGuide();
    main_layout->setCurrentWidget(onboardingWindow);
  });
  QObject::connect(settingsWindow, &SettingsWindow::showDriverView, [=] {
    homeWindow->showDriverView(true);
  });

  onboardingWindow = new OnboardingWindow(this);
  main_layout->addWidget(onboardingWindow);
  QObject::connect(onboardingWindow, &OnboardingWindow::onboardingDone, [=]() {
    main_layout->setCurrentWidget(homeWindow);
  });
  if (!onboardingWindow->completed()) {
    main_layout->setCurrentWidget(onboardingWindow);
  }

  QObject::connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    if (!offroad) {
      closeSettings();
    }
  });
  // 监测档位变化，两次连续挂 P 档触发关机
  QObject::connect(uiState(), &UIState::uiUpdate, this, &MainWindow::updateState);
  QObject::connect(device(), &Device::interactiveTimeout, [=]() {
    if (main_layout->currentWidget() == settingsWindow) {
      closeSettings();
    }
  });

  // load fonts
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-Black.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-Bold.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-ExtraBold.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-ExtraLight.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-Medium.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-Regular.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-SemiBold.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/Inter-Thin.ttf");
  QFontDatabase::addApplicationFont("../assets/fonts/JetBrainsMono-Medium.ttf");

  // no outline to prevent the focus rectangle
  setStyleSheet(R"(
    * {
      font-family: Inter;
      outline: none;
    }
  )");
  setAttribute(Qt::WA_NoSystemBackground);
}

void MainWindow::openSettings(int index, const QString &param) {
  main_layout->setCurrentWidget(settingsWindow);
  settingsWindow->setCurrentPanel(index, param);
}

void MainWindow::closeSettings() {
  main_layout->setCurrentWidget(homeWindow);

  if (uiState()->scene.started) {
    homeWindow->showSidebar(false);
  }
}

void MainWindow::updateState(const UIState &s) {
  // 监测档位变化：两次连续挂 P 档（3 秒内）触发关机
  const SubMaster &sm = *(s.sm);
  if (!sm.alive("carState")) return;
  auto car_state = sm["carState"].getCarState();
  bool is_p_gear = (car_state.getGearShifter() == cereal::CarState::GearShifter::PARK);

  // 检测 P 档上升沿（从非 P 变为 P）
  if (is_p_gear && !was_p_gear) {
    qint64 now = millis_since_boot();
    if (last_p_gear_ts > 0 && (now - last_p_gear_ts) < P_GEAR_DOUBLE_TAP_MS) {
      // 两次挂 P 档在时间窗口内，触发关机
      doPoweroff();
      last_p_gear_ts = 0;
    } else {
      last_p_gear_ts = now;
    }
  }
  // 超过时间窗口未第二次挂 P 档，重置
  if (last_p_gear_ts > 0 && (millis_since_boot() - last_p_gear_ts) > P_GEAR_DOUBLE_TAP_MS) {
    last_p_gear_ts = 0;
  }
  was_p_gear = is_p_gear;
}

void MainWindow::doPoweroff() {
  // 辅助驾驶 engaged 时不允许关机
  if (uiState()->engaged()) {
    printf("[Poweroff] Engaged, skip poweroff\n");
    return;
  }

  // 显示关机提示
  QLabel *overlay = new QLabel("两次挂P档，正在关机...", this);
  overlay->setStyleSheet("background-color: rgba(0,0,0,0.85); color: white; font-size: 48px; font-weight: bold;");
  overlay->setAlignment(Qt::AlignCenter);
  overlay->setGeometry(0, 0, width(), height());
  overlay->show();
  overlay->raise();

  Params params;
  params.putBool("DoShutdown", true);
  // 立即执行关机脚本（PILOT_ROOT 为项目根目录，由启动脚本导出）
  std::system("(python3 $PILOT_ROOT/scripts/shutdown_pc.py) &");
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  bool ignore = false;
  switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove: {
      // ignore events when device is awakened by resetInteractiveTimeout
      ignore = !device()->isAwake();
      device()->resetInteractiveTimeout();
      break;
    }
    default:
      break;
  }
  return ignore;
}
