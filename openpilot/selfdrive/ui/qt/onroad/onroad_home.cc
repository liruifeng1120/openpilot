#include "selfdrive/ui/qt/onroad/onroad_home.h"

#include <QPainter>
#include <QStackedLayout>


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QDialog>
#include <QMouseEvent>
#include <thread>

#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/carrot.h"
#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_helpers.h"
#include "selfdrive/ui/qt/maps/map_panel.h"
#endif

class OverlayDialog : public QWidget {
  Q_OBJECT

public:
  explicit OverlayDialog(QWidget* parent = nullptr) : QWidget(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setStyleSheet("background-color: rgba(0, 0, 0, 0.8); border-radius: 10px;");
    resize(400, 300);
  }

  void setContent(QWidget* content) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(content);
    layout->setMargin(0);
    setLayout(layout);
  }
};

OnroadWindow::OnroadWindow(QWidget *parent) : QOpenGLWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  //main_layout->setContentsMargins(UI_BORDER_SIZE, 0, UI_BORDER_SIZE, 0);

  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, this);
    split->insertWidget(0, arCam);
  }

  if (getenv("MAP_RENDER_VIEW")) {
    CameraWidget *map_render = new CameraWidget("navd", VISION_STREAM_MAP, this);
    split->insertWidget(0, map_render);
  }

  stacked_layout->addWidget(split_wrapper);

  // ���Ӻ��Ӿ�/ä�����棺���л�������ʾ
  // �������κβ��֣���Ϊ OnroadWindow �������Ӵ��ڣ��� resizeEvent ��λ������/���Ͻǡ�
  // ����ת�����ʾ��ä�����棬����ת�����ʾ��ä�����棬Ĭ�����ء�
  blind_left = new CameraWidget("cam_blindspot", VISION_STREAM_BLIND_LEFT, this);
  blind_right = new CameraWidget("cam_blindspot", VISION_STREAM_BLIND_RIGHT, this);
  blind_left->setVisible(false);
  blind_right->setVisible(false);

  // �ֻ�Ͷ�� (scrcpy)����������, ����Ĭ������, �Զ���ⰲ׿�豸���Ӻ���ʾ��
  // - ���ӳɹ� �� �Զ���ʾ (�����û���˫������)
  // - �Ͽ� �� �Զ�����, ������ѯ�ȴ�����
  // - ˫�������� �� ����; ˫��������հ� �� �ָ���ʾ
  phone_mirror = new ScrcpyWidget(this);
  phone_mirror->setVisible(false);
  // �����Զ���� (2s ��ѯ adb devices), ����δ����ʱ��������
  phone_mirror->setAutoConnect(true);
  // ��ӡ scrcpy �ź���־�������
  QObject::connect(phone_mirror, &ScrcpyWidget::errorOccurred, [](const QString &msg) {
    fprintf(stderr, "[scrcpy] ERROR: %s\n", msg.toUtf8().constData());
  });
  QObject::connect(phone_mirror, &ScrcpyWidget::connected, [this](int w, int h) {
    fprintf(stderr, "[scrcpy] connected: %dx%d\n", w, h);
    // �״�����: ���ֻ�ʵ�ʻ������, �����½Ƿ���, ֮�������û��϶�/����
    if (!phone_mirror_placed_) {
      const int title_h = phone_mirror->titleBarHeight();
      const int margin = 5;
      const int border = UI_BORDER_SIZE;  // ��ɫ�߿���� 30px, Ͷ������������ƫ�Ʊܿ�
      // Ĭ�Ͽ���ȡ��Ļ 1/5, ���ֻ����������߶�
      int mw = width() / 5;
      int video_h = (w > 0) ? mw * h / w : mw * 16 / 9;
      int mh = title_h + video_h;
      // ���ܸ߳���, ����Ļ�߶� 60% ����, ���ƿ���
      int max_h = height() * 6 / 10;
      if (mh > max_h) {
        mh = max_h;
        video_h = mh - title_h;
        mw = (h > 0) ? video_h * w / h : video_h * 9 / 16;
      }
      // ���½�, ������ƫ�� UI_BORDER_SIZE �ܿ���ɫ�߿�
      int mx = border + margin;
      int my = height() - mh - border - margin;
      phone_mirror->setGeometry(mx, my, mw, mh);
      phone_mirror_placed_ = true;
    }
    // �û�δ������������ʾ
    if (!phone_mirror_user_hidden_) {
      phone_mirror->setVisible(true);
      phone_mirror->raise();
    }
  });
  QObject::connect(phone_mirror, &ScrcpyWidget::disconnected, [this]() {
    fprintf(stderr, "[scrcpy] disconnected\n");
    // �Ͽ�: ���ش���, ��λ�û����ر�� (�´������Զ���ʾ)
    phone_mirror->setVisible(false);
    phone_mirror_user_hidden_ = false;
    phone_mirror_auto_shown_ = false;
    phone_mirror_placed_ = false;
    phone_mirror_maximized_ = false;
  });

  // 分辨率变化 (手机旋转) 时重新计算 widget 尺寸, 保持吸附在左侧
  auto recalcPhoneMirror = [this](int w, int h) {
    if (phone_mirror_maximized_) {
      // 最大化模式: 维持竖屏全高, 按新比例缩放宽度, 始终吸附在左侧
      const int border = 5;
      int mh = this->height() - border * 2;
      int mw = (w > 0 && h > 0) ? mh * w / h : mh * 9 / 16;
      mw = std::min(mw, this->width() - border * 2);
      int mx = border;
      phone_mirror->setGeometry(mx, border, mw, mh);
      phone_mirror->raise();
      fprintf(stderr, "[scrcpy] maximized geometry recalculated: %dx%d+%d+%d (video %dx%d)\n", mw, mh, mx, border, w, h);
      return;
    }
    const QRect old_geo = phone_mirror->geometry();
    const int cx = old_geo.x() + old_geo.width() / 2;
    const int cy = old_geo.y() + old_geo.height() / 2;

    const int title_h = phone_mirror->titleBarHeight();
    const int margin = 5;
    const int border = UI_BORDER_SIZE;
    int mw = width() / 5;
    int video_h = (w > 0) ? mw * h / w : mw * 16 / 9;
    int mh = title_h + video_h;
    int max_h = height() * 6 / 10;
    if (mh > max_h) {
      mh = max_h;
      video_h = mh - title_h;
      mw = (h > 0) ? video_h * w / h : video_h * 9 / 16;
    }
    int mx = cx - mw / 2;
    int my = cy - mh / 2;
    mx = std::max(border + margin, std::min(mx, width() - mw - border - margin));
    my = std::max(border + margin, std::min(my, height() - mh - border - margin));
    phone_mirror->setGeometry(mx, my, mw, mh);
    phone_mirror->raise();
    fprintf(stderr, "[scrcpy] geometry recalculated: %dx%d+%d+%d (video %dx%d)\n", mw, mh, mx, my, w, h);
  };
  QObject::connect(phone_mirror, &ScrcpyWidget::resolutionChanged, this, recalcPhoneMirror);
  QObject::connect(phone_mirror, &ScrcpyWidget::toggleRequested, [this]() {
    // ˫��������: ���ش��� (Ͷ����������, ˫��������ָ�)
    phone_mirror->setVisible(false);
    phone_mirror_user_hidden_ = true;
    phone_mirror_auto_shown_ = false;
    fprintf(stderr, "[scrcpy] user hidden via double-click\n");
  });
  QObject::connect(phone_mirror, &ScrcpyWidget::maximizeToggled, [this](bool maximized) {
    phone_mirror_maximized_ = maximized;
    fprintf(stderr, "[scrcpy] %s\n", maximized ? "maximized" : "restored");
  });

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // setup stacking order
  alerts->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);
}

void OnroadWindow::updateState(const UIState &s) {
  UIState* ss = uiState();

  // phone mirror param monitoring
  {
    Params params;
    int phone_mirror_cmd = params.getInt("PhoneMirror");
    static int phone_mirror_cmd_last = 0;
    if (phone_mirror_cmd != phone_mirror_cmd_last) {
      phone_mirror_cmd_last = phone_mirror_cmd;
      fprintf(stderr, "[scrcpy] PhoneMirror param=%d\n", phone_mirror_cmd);
      if (phone_mirror_cmd == 1 && phone_mirror) {
        phone_mirror->setAutoConnect(true);
      }
      else if (phone_mirror_cmd == 0 && phone_mirror) {
        phone_mirror->setAutoConnect(false);
        phone_mirror->setVisible(false);
        phone_mirror_user_hidden_ = false;
        if (phone_mirror->isRunning()) phone_mirror->stop();
        phone_mirror_on_ = false;
      }
    }
    // apply opacity every frame (user may change in settings)
    if (phone_mirror) {
      phone_mirror->setOpacity(params.getInt("PhoneMirrorOpacity"));
    }
  }

  // auto-show phone mirror near intersections
  if (phone_mirror && phone_mirror->isRunning()) {
    Params p;
    if (p.getInt("PhoneMirrorAutoShow") == 1) {
      const SubMaster& sm = *(s.sm);
      if (sm.alive("carrotMan")) {
        const auto& carrot = sm["carrotMan"].getCarrotMan();
        int turn_info = carrot.getXTurnInfo();
        int dist_to_turn = carrot.getXDistToTurn();
        bool near_intersection = (turn_info == 1 || turn_info == 2 || turn_info == 5 || turn_info == 7) &&
                                 dist_to_turn > 0 && dist_to_turn < 200;
        if (near_intersection && !phone_mirror->isVisible()) {
          phone_mirror->setVisible(true);
          phone_mirror->raise();
          phone_mirror_auto_shown_ = true;
          phone_mirror_user_hidden_ = false;
          fprintf(stderr, "[scrcpy] auto-show near intersection (dist=%d, turn=%d)\n", dist_to_turn, turn_info);
        } else if (!near_intersection && phone_mirror_auto_shown_ && phone_mirror->isVisible()) {
          phone_mirror->setVisible(false);
          phone_mirror_auto_shown_ = false;
          fprintf(stderr, "[scrcpy] auto-hide after intersection\n");
        }
      }
    }
  }

  if (!s.scene.started) {
    ss->scene._current_carrot_display_prev = -1;
    // �³�ʱ����ä������
    if (blind_left) blind_left->setVisible(false);
    if (blind_right) blind_right->setVisible(false);
    // �³�ʱ���Զ��ر��ֻ�Ͷ�� (�û���������ͣ��ʱ��)
    return;
  }

  //alerts->updateState(s);
  ui_update_alert(OnroadAlerts::getAlert(*(s.sm), s.scene.started_frame));
  if (s.scene.map_on_left) {
    split->setDirection(QBoxLayout::LeftToRight);
  } else {
    split->setDirection(QBoxLayout::RightToLeft);
  }
  nvg->updateState(s);

  // ���Ӻ��Ӿ�������ת���״̬��̬��ʾ/��������ä������
  // ����ת��� �� ��ʾ��ä��������ת��� �� ��ʾ��ä�������ر� �� ����
  // �ҵ��� R �� ����ä������ͬʱ��ʾ
  {
    const SubMaster& sm = *(s.sm);
    auto car_state = sm["carState"].getCarState();
    bool left_blink = car_state.getLeftBlinker();
    bool right_blink = car_state.getRightBlinker();
    // ���� carrot.cc �� fork/turn/atc ����ת��Ҳ�� blinker ���߼�
    if (sm.alive("carrotMan")) {
      auto carrot = sm["carrotMan"].getCarrotMan();
      std::string atc = carrot.getAtcType();
      if (atc == "fork left" || atc == "turn left" || atc == "atc left") left_blink = true;
      if (atc == "fork right" || atc == "turn right" || atc == "atc right") right_blink = true;
    }
    // �ҵ��� R ʱ����ä������ͬʱ��ʾ
    bool reverse_gear = (car_state.getGearShifter() == cereal::CarState::GearShifter::REVERSE);
    if (blind_left) {
      blind_left->setVisible(left_blink || reverse_gear);
      // 盲区后视镜需要覆盖在导航 (mapDialog) 之上, 显示时主动 raise
      if (blind_left->isVisible()) blind_left->raise();
    }
    if (blind_right) {
      blind_right->setVisible(right_blink || reverse_gear);
      if (blind_right->isVisible()) blind_right->raise();
    }
  }

  QColor bgColor = bg_colors[s.status];
  QColor bgColor_long = bg_colors[s.status];
  const SubMaster& sm = *(s.sm);
  const auto car_control = sm["carControl"].getCarControl();
  auto selfdrive_state = sm["selfdriveState"].getSelfdriveState();

  //if (s.status == STATUS_DISENGAGED && car_control.getLatActive()) {
  //    bgColor = bg_colors[STATUS_LAT_ACTIVE];
  //}
  const auto car_state = sm["carState"].getCarState();
  if (car_state.getSteeringPressed()) {
      bgColor = bg_colors[STATUS_OVERRIDE];
  }
  else if (car_control.getLatActive()) {
      bgColor = bg_colors[STATUS_ENGAGED];
  }
  else if (car_state.getLatEnabled()) {
      bgColor = bg_colors[STATUS_ACTIVE];
  }
  else
      bgColor = bg_colors[STATUS_DISENGAGED];

  if (car_state.getGasPressed()) {
      bgColor_long = bg_colors[STATUS_OVERRIDE];
  }
  else if (selfdrive_state.getEnabled()) {
      bgColor_long = bg_colors[STATUS_ENGAGED];
  }
  else if (car_state.getCruiseState().getAvailable()) {
	  bgColor_long = bg_colors[STATUS_ACTIVE];
  }
  else
      bgColor_long = bg_colors[STATUS_DISENGAGED];
  if (bg != bgColor || bg_long != bgColor_long) {
    // repaint border
    bg = bgColor;
    bg_long = bgColor_long;
    //update();
  }
  update();
  if (true) {
      int carrot_display = 0;

      static int carrot_cmd_index_last = 0;
      if (sm.alive("carrotMan")) {
        const auto& carrot = sm["carrotMan"].getCarrotMan();
        int carrot_cmd_index = carrot.getCarrotCmdIndex();
        if (carrot_cmd_index != carrot_cmd_index_last) {
          carrot_cmd_index_last = carrot_cmd_index;
          QString carrot_cmd = QString::fromStdString(carrot.getCarrotCmd());
          QString carrot_arg = QString::fromStdString(carrot.getCarrotArg());
          if (carrot_cmd == "DISPLAY") {
            if (carrot_arg == "TOGGLE") {
              carrot_display = 5;
              //printf("Display toggle\n");
            }
            else if (carrot_arg == "DEFAULT") {
              carrot_display = 1;
              //printf("Display 1\n");
            }
            else if (carrot_arg == "ROAD") {
              carrot_display = 2;
              //printf("Display 2\n");
            }
            else if (carrot_arg == "MAP") {
              carrot_display = 3;
              //printf("Display 3\n");
            }
            else if (carrot_arg == "FULLMAP") {
              carrot_display = 4;
              //printf("Display 4\n");
            }
          }
        }
      }

      if (carrot_display == 5) ss->scene._current_carrot_display = (ss->scene._current_carrot_display % 3) + 1;
      else if(carrot_display > 0) ss->scene._current_carrot_display = carrot_display;
      if (map == nullptr && ss->scene._current_carrot_display > 2) ss->scene._current_carrot_display = 1;
      //if (offroad) _current_carrot_display = 1;
      switch (ss->scene._current_carrot_display) {
      case 1: // default
          if (map != nullptr) map->setVisible(false);
          if (ss->scene._current_carrot_display_prev != ss->scene._current_carrot_display) ss->scene._display_time_count = 100; // 100: about 5 seconds
          if (ss->scene._display_time_count-- <= 0) ss->scene._current_carrot_display = 2; // change to road view
          break;
      case 2: // road
          if (map != nullptr) map->setVisible(false);
          break;
      case 3: // map
          if (map == nullptr) ss->scene._current_carrot_display = 1;
          else {
              map->setVisible(true);
              //map->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
          }
          break;
      case 4: // fullmap
          if (map == nullptr) ss->scene._current_carrot_display = 1;
          else {
              map->setVisible(true);
              //map->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
              //map->setFixedWidth(topWidget(this)->width() - UI_BORDER_SIZE);
          }
          break;
      }
      ss->scene._current_carrot_display_prev = ss->scene._current_carrot_display;
  }

}

void OnroadWindow::mousePressEvent(QMouseEvent* e) {
  //printf("uiState()->scene.navigate_on_openpilot = %d\n", uiState()->scene.navigate_on_openpilot);
//#ifdef ENABLE_MAPS
//  if (map != nullptr) {
    // Switch between map and sidebar when using navigate on openpilot
    //bool sidebarVisible = geometry().x() > 0;
    //bool show_map = uiState()->scene.navigate_on_openpilot ? sidebarVisible : !sidebarVisible;
    //map->setVisible(show_map && !map->isVisible());
//  }
//#endif
  // propagation event to parent(HomeWindow)
  int x = e->x();   // 430 - 500 : gap window
  int y = height() - e->y();  // 60 - 180 : gap window
  int ey = e->y();
  printf("x=%d, y=%d, ey=%d\n", x, y, ey);
  double now = millis_since_boot();
  static double last_click_time = 0;
  static int _click_count = 0;
  // 40,150, 200, 150
  Params	params;
  if (x > 40 && x < 370 && ey > 30 && ey < 240) {   // date & time
    int show_date_time = params.getInt("ShowDateTime");
    params.putIntNonBlocking("ShowDateTime", (show_date_time + 1) % 3);
  }
  else if (x > 40 && x < 500 && y > 400 && y < 530) {   // device info
    int show_device_state = params.getInt("ShowDeviceState");
    params.putIntNonBlocking("ShowDeviceState", (show_device_state + 1) % 2);
  }
  else if (x > 40 && x < 200 && y > 20 && y < 150) {   // driving mode
    int my_driving_mode = params.getInt("MyDrivingMode");
    params.putIntNonBlocking("MyDrivingMode", (my_driving_mode) % 4 + 1);
  }
  else if (x > 350 && x < 550 && y > 20 && y < 250) { // gap control
    int longitudinalPersonalityMax = params.getInt("LongitudinalPersonalityMax");
    int personality = (params.getInt("LongitudinalPersonality") - 1 + longitudinalPersonalityMax) % longitudinalPersonalityMax;
    params.putIntNonBlocking("LongitudinalPersonality", personality);

  }
  else {
    if (now - last_click_time < 500) {
      _click_count++;
    }
    else {
      _click_count = 0;
    }
    last_click_time = now;
    if (_click_count == 3) {
      params.putIntNonBlocking("SoftRestartTriggered", 1);
    }

    UIState* s = uiState();
    s->scene._current_carrot_display = (s->scene._current_carrot_display % 3) + 1;  // cycle 1-4
    printf("_current_carrot_display1=%d\n", s->scene._current_carrot_display);
    QWidget::mousePressEvent(e);
  }
}
//OverlayDialog* mapDialog = nullptr;
void OnroadWindow::offroadTransition(bool offroad) {
#ifdef ENABLE_MAPS
  if (!offroad) {
    if (map == nullptr && (!MAPBOX_TOKEN.isEmpty())) {
      printf("####### Initialize MapPanel\n");
#if 0
      auto m = new MapPanel(get_mapbox_settings());
      map = m;

      m->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
      split->insertWidget(0, m);

      // hidden by default, made visible when navRoute is published
      m->setVisible(false);
#else
      OverlayDialog* mapDialog = new OverlayDialog(this);
      mapDialog->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
      mapDialog->setAttribute(Qt::WA_TranslucentBackground);
      mapDialog->setAttribute(Qt::WA_NoSystemBackground);

      // add MapPanel inside overlay dialog
      auto m = new MapPanel(get_mapbox_settings(), mapDialog);
      map = m;
      mapDialog->setContent(m);

      // position at specific screen location
      mapDialog->setGeometry(topWidget(this)->width() - 790 - UI_BORDER_SIZE, UI_BORDER_SIZE + 15, 775, topWidget(this)->height() - 400);

      //mapDialog->hide(); // hidden by default initially
      mapDialog->show();
      mapDialog->raise();
      uiState()->scene._current_carrot_display = 1;

#endif
    }
  }
#endif
  alerts->clear();
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.beginNativePainting();
    UIState* s = uiState();
    extern void ui_draw_border(UIState * s, int w, int h, QColor bg, QColor bg_long);
    ui_draw_border(s, width(), height(), bg, bg_long);
    p.endNativePainting();
}

void OnroadWindow::resizeEvent(QResizeEvent *event) {
    QOpenGLWidget::resizeEvent(event);
    // ���л�ä�����棺��ä�����������Ͻǣ���ä�����������Ͻǡ�
    // ������� 4:3��640x480�������Ȱ����� 1/6 * 1.8 ���㣨ԭ 1/6 ƫС���Ŵ� 1.8 ������
    int pip_w = (int)(width() / 6 * 1.8);
    int pip_h = pip_w * 3 / 4;  // 4:3
    if (pip_w > 0 && pip_h > 0) {
        if (blind_left) {
            blind_left->setGeometry(UI_BORDER_SIZE, UI_BORDER_SIZE, pip_w, pip_h);
            blind_left->raise();
        }
        if (blind_right) {
            blind_right->setGeometry(width() - pip_w - UI_BORDER_SIZE, UI_BORDER_SIZE, pip_w, pip_h);
            blind_right->raise();
        }
    }
    // �ֻ�Ͷ���������״�δ����ʱ��Ĭ��λ��; �û��϶�/���ź������伸��,
    // ֻ�ڸ���������ʱ�Ѵ������ƻؿɼ���Χ��
    if (phone_mirror && phone_mirror_placed_) {
        if (phone_mirror_maximized_ && phone_mirror->isRunning()) {
            // 最大化模式: 父窗口缩放时维持竖屏全高, 按比例缩放宽度, 始终吸附在左侧
            int vw = phone_mirror->videoWidth();
            int vh = phone_mirror->videoHeight();
            if (vw > 0 && vh > 0) {
                const int border = 5;
                int mh = height() - border * 2;
                int mw = mh * vw / vh;
                mw = std::min(mw, width() - border * 2);
                int mx = border;
                phone_mirror->setGeometry(mx, border, mw, mh);
                phone_mirror->raise();
            }
        } else {
            QRect g = phone_mirror->geometry();
            // ��֤���ٱ����� + һ���ֿɼ�
            if (g.right() < 40 || g.bottom() < 28 || g.left() > width() - 40 || g.top() > height() - 28) {
                // ��ȫ�ܵ��ɼ�����, ���û�Ĭ�����½� (����ǰ��Ƶ����)
                int vw = phone_mirror->videoWidth();
                int vh = phone_mirror->videoHeight();
                const int title_h = phone_mirror->titleBarHeight();
                const int margin = 5;
                const int border = UI_BORDER_SIZE;
                int mw = width() / 5;
                int video_h = (vw > 0) ? mw * vh / vw : mw * 16 / 9;
                int mh = title_h + video_h;
                int max_h = height() * 6 / 10;
                if (mh > max_h) {
                  mh = max_h;
                  video_h = mh - title_h;
                  mw = (vh > 0) ? video_h * vw / vh : video_h * 9 / 16;
                }
                phone_mirror->setGeometry(border + margin, height() - mh - border - margin, mw, mh);
            }
        }
        phone_mirror->raise();
    }
}

// ˫��������հ�����: �ָ����û����ص��ֻ�Ͷ������ (Ͷ����������)
void OnroadWindow::mouseDoubleClickEvent(QMouseEvent* e) {
  if (phone_mirror && phone_mirror->isRunning() && phone_mirror_user_hidden_) {
    phone_mirror->setVisible(true);
    phone_mirror->raise();
    phone_mirror_user_hidden_ = false;
    fprintf(stderr, "[scrcpy] user show via double-click\n");
    return;
  }
  QWidget::mouseDoubleClickEvent(e);
}

// OnroadWindow OpenGL init stubs
void OnroadWindow::initializeGL() {
    initializeOpenGLFunctions();

    // init nanovg context from parent widget
    //s->vg = nvgCreate(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    //if (s->vg == nullptr) {
    //    printf("Could not init nanovg.\n");
    //    return;
    //}
}


#include "selfdrive/ui/qt/onroad/onroad_home.moc"
