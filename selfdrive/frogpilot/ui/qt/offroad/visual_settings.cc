#include "selfdrive/frogpilot/ui/qt/offroad/visual_settings.h"

FrogPilotVisualsPanel::FrogPilotVisualsPanel(SettingsWindow *parent) : FrogPilotListWidget(parent) {
  std::string branch = params.get("GitBranch");
  isRelease = branch == "FrogPilot";

  const std::vector<std::tuple<QString, QString, QString, QString>> visualToggles {
    {"AlertVolumeControl", tr("警报音量控制"), tr("控制openpilot中每个单独声音的音量级别。"), "../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("解除音量"), tr("相关警报：\n\n自适应巡航已禁用\n驻车制动已启用\n刹车踏板被按下\n速度过低"), ""},
    {"EngageVolume", tr("启用音量"), tr("相关警报：\n\nNNFF扭矩控制器已加载\nopenpilot已启用"), ""},
    {"PromptVolume", tr("提示音量"), tr("相关警报：\n\n在盲区检测到车辆\n速度过低\n转向不可用低于'X'\n接管，转向超过转向限制"), ""},
    {"PromptDistractedVolume", tr("提示分心音量"), tr("相关警报：\n\n注意，驾驶员分心\n触摸方向盘，驾驶员无反应"), ""},
    {"RefuseVolume", tr("拒绝音量"), tr("相关警报：\n\nopenpilot不可用"), ""},
    {"WarningSoftVolume", tr("警告软音量"), tr("相关警报：\n\n刹车！风险碰撞\n立即接管"), ""},
    {"WarningImmediateVolume", tr("警告立即音量"), tr("相关警报：\n\n立即解除，驾驶员分心\n立即解除，驾驶员无反应"), ""},

    {"CustomAlerts", tr("自定义警报"), tr("启用openpilot事件的自定义警报。"), "../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GreenLightAlert", tr("绿灯警报"), tr("当交通信号灯从红色变为绿色时获取警报。"), ""},
    {"LeadDepartingAlert", tr("前车离开警报"), tr("当前车在静止时开始离开时获取警报。"), ""},
    {"LoudBlindspotAlert", tr("响亮盲区警报"), tr("在尝试变道时，当在盲区检测到车辆时启用更响亮的警报。"), ""},

    {"CustomUI", tr("自定义道路UI"), tr("自定义道路UI。"), "../assets/offroad/icon_road.png"},
    {"Compass", tr("指南针"), tr("在道路UI中添加一个指南针。"), ""},
    {"CustomPaths", tr("路径"), tr("显示您在驾驶路径上的预计加速度，检测到的相邻车道，或在盲区检测到的车辆。"), ""},
    {"PedalsOnUI", tr("踏板被按下"), tr("在道路UI中显示刹车和油门踏板，位于方向盘图标下方。"), ""},
    {"RoadNameUI", tr("道路名称"), tr("在屏幕底部显示当前道路的名称。来源于OpenStreetMap。"), ""},
    {"WheelIcon", tr("方向盘图标"), tr("用自定义图标替换默认的方向盘图标。"), ""},
    {"CustomTheme", tr("自定义主题"), tr("启用使用自定义主题的功能。"), "../frogpilot/assets/wheel_images/frog.png"},
    {"CustomColors", tr("颜色主题"), tr("用主题颜色替换标准的openpilot配色方案。\n\n想提交自己的配色方案吗？请在FrogPilot Discord的'feature-request'频道发布！"), ""},
    {"CustomIcons", tr("图标包"), tr("用一组主题图标替换标准的openpilot图标。\n\n想提交自己的图标包吗？请在FrogPilot Discord的'feature-request'频道发布！"), ""},
    {"CustomSounds", tr("声音包"), tr("用一组主题声音替换标准的openpilot声音。\n\n想提交自己的声音包吗？请在FrogPilot Discord的'feature-request'频道发布！"), ""},
    {"CustomSignals", tr("转向信号"), tr("为您的转向信号添加主题动画。\n\n想提交自己的转向信号动画吗？请在FrogPilot Discord的'feature-request'频道发布！"), ""},
    {"HolidayThemes", tr("节日主题"), tr("openpilot主题根据当前/即将到来的节日进行更改。小节日持续一天，而大节日（复活节、圣诞节、万圣节等）持续一周。"), ""},
    {"RandomEvents", tr("随机事件"), tr("在某些驾驶条件下享受随机事件带来的不确定性。这纯粹是外观上的变化，不会影响驾驶控制！"), ""},

    {"DeveloperUI", tr("开发者UI"), tr("获取openpilot在后台执行的各种详细信息。"), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"BorderMetrics", tr("边界指标"), tr("在onroad UI边界中显示指标。"), ""},
    {"FPSCounter", tr("FPS计数器"), tr("显示您的onroad UI的'每秒帧数'（FPS），以监控系统性能。"), ""},
    {"LateralMetrics", tr("横向指标"), tr("显示与openpilot横向性能相关的各种指标。"), ""},
    {"LongitudinalMetrics", tr("纵向指标"), tr("显示与openpilot纵向性能相关的各种指标。"), ""},
    {"NumericalTemp", tr("数值温度计"), tr("用基于内存、CPU和GPU之间最高温度的数值温度计替换'良好'、'正常'和'高'的温度状态。"), ""},
    {"SidebarMetrics", tr("侧边栏"), tr("在侧边栏中显示CPU、GPU、RAM、IP和已用/剩余存储的各种自定义指标。"), ""},
    {"UseSI", tr("使用国际单位制"), tr("以SI格式显示相关指标。"), ""},

    {"ModelUI", tr("模型UI"), tr("自定义屏幕上的模型可视化。"), "../assets/offroad/icon_calibration.png"},
    {"DynamicPathWidth", tr("动态路径宽度"), tr("根据openpilot的当前参与状态动态调整路径宽度。"), ""},
    {"HideLeadMarker", tr("隐藏前导标记"), tr("从onroad UI中隐藏前导标记。"), ""},
    {"LaneLinesWidth", tr("车道线"), tr("调整显示中车道线的视觉厚度。\n\n默认值与MUTCD平均值4英寸相匹配。"), ""},
    {"PathEdgeWidth", tr("路径边缘"), tr("调整在您的UI上显示的路径边缘的宽度，以表示不同的驾驶模式和状态。\n\n默认值为总路径的20%。\n\n蓝色=导航\n浅蓝色='始终开启的横向'\n绿色=默认\n橙色='实验模式'\n红色='交通模式'\n黄色='条件实验模式'覆盖"), ""},
    {"PathWidth", tr("路径宽度"), tr("自定义在您的UI上显示的驾驶路径的宽度。\n\n默认值与2019年雷克萨斯ES 350的宽度相匹配。"), ""},
    {"RoadEdgesWidth", tr("道路边缘"), tr("调整显示中道路边缘的视觉厚度。\n\n默认值为MUTCD平均车道线宽度4英寸的一半。"), ""},
    {"UnlimitedLength", tr("'无限'道路UI长度"), tr("将路径、车道线和道路边缘的显示扩展到模型可以看到的最远处。"), ""},

    {"QOLVisuals", tr("生活质量"), tr("各种生活质量的变化，以改善您的整体openpilot体验。"), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"BigMap", tr("大地图"), tr("增加onroad UI中地图的大小。"), ""},
    {"CameraView", tr("相机视图"), tr("选择您在onroad UI中偏好的相机视图。这纯粹是视觉上的变化，不会影响openpilot的驾驶方式。"), ""},
    {"DriverCamera", tr("倒车时驾驶员相机"), tr("在倒车时显示驾驶员相机画面。"), ""},
    {"HideSpeed", tr("隐藏速度"), tr("在onroad UI中隐藏速度指示器。额外的切换允许通过点击速度本身来隐藏/显示。"), ""},
    {"MapStyle", tr("地图样式"), tr("选择用于导航的地图样式。"), ""},
    {"WheelSpeed", tr("使用车轮速度"), tr("在onroad UI中使用车轮速度而不是仪表盘速度。"), ""},

    {"ScreenManagement", tr("屏幕管理"), tr("管理屏幕的亮度、超时设置，并隐藏onroad UI元素。"), "../frogpilot/assets/toggle_icons/icon_light.png"},
    {"HideUIElements", tr("隐藏UI元素"), tr("从onroad屏幕中隐藏选定的UI元素。"), ""},
    {"ScreenBrightness", tr("屏幕亮度"), tr("自定义离线时的屏幕亮度。"), ""},
    {"ScreenBrightnessOnroad", tr("屏幕亮度（在路上）"), tr("自定义在路上时的屏幕亮度。"), ""},
    {"ScreenRecorder", tr("屏幕录制器"), tr("启用在路上录制屏幕的功能。"), ""},
    {"ScreenTimeout", tr("屏幕超时"), tr("自定义屏幕关闭所需的时间。"), ""},
    {"ScreenTimeoutOnroad", tr("屏幕超时（在路上）"), tr("自定义在路上时屏幕关闭所需的时间。"), ""},
    {"StandbyMode", tr("待机模式"), tr("在路上时，屏幕超时后关闭，但在参与状态变化或重要警报触发时重新唤醒。"), ""},
  };

  for (const auto &[param, title, desc, icon] : visualToggles) {
    AbstractControl *toggle;

    if (param == "AlertVolumeControl") {
      FrogPilotParamManageControl *alertVolumeControlToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(alertVolumeControlToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end());
        }
      });
      toggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      if (param == "WarningImmediateVolume") {
        toggle = new FrogPilotParamValueControl(param, title, desc, icon, 25, 100, std::map<int, QString>(), this, false, "%");
      } else {
        toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, "%");
      }

    } else if (param == "CustomAlerts") {
      FrogPilotParamManageControl *customAlertsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customAlertsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedCustomAlertsKeys = customAlertsKeys;

          if (!hasBSM) {
            modifiedCustomAlertsKeys.erase("LoudBlindspotAlert");
          }

          toggle->setVisible(modifiedCustomAlertsKeys.find(key.c_str()) != modifiedCustomAlertsKeys.end());
        }
      });
      toggle = customAlertsToggle;

    } else if (param == "CustomTheme") {
      FrogPilotParamManageControl *customThemeToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customThemeToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customThemeKeys.find(key.c_str()) != customThemeKeys.end());
        }
      });
      toggle = customThemeToggle;
    } else if (param == "CustomColors" || param == "CustomIcons" || param == "CustomSignals" || param == "CustomSounds") {
      std::vector<QString> themeOptions{tr("Stock"), tr("Frog"), tr("Tesla"), tr("Stalin")};
      FrogPilotButtonParamControl *themeSelection = new FrogPilotButtonParamControl(param, title, desc, icon, themeOptions);
      toggle = themeSelection;

      if (param == "CustomSounds") {
        QObject::connect(static_cast<FrogPilotButtonParamControl*>(toggle), &FrogPilotButtonParamControl::buttonClicked, [this](int id) {
          if (id == 1) {
            if (FrogPilotConfirmationDialog::yesorno(tr("Do you want to enable the bonus 'Goat' sound effect?"), this)) {
              params.putBoolNonBlocking("GoatScream", true);
            } else {
              params.putBoolNonBlocking("GoatScream", false);
            }
          }
        });
      }

    } else if (param == "CustomUI") {
      FrogPilotParamManageControl *customUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedCustomOnroadUIKeys = customOnroadUIKeys;

          if (!hasOpenpilotLongitudinal && !hasAutoTune) {
            modifiedCustomOnroadUIKeys.erase("DeveloperUI");
          }

          toggle->setVisible(modifiedCustomOnroadUIKeys.find(key.c_str()) != modifiedCustomOnroadUIKeys.end());
        }
      });
      toggle = customUIToggle;
    } else if (param == "CustomPaths") {
      std::vector<QString> pathToggles{"AccelerationPath", "AdjacentPath", "BlindSpotPath", "AdjacentPathMetrics"};
      std::vector<QString> pathToggleNames{tr("Acceleration"), tr("Adjacent"), tr("Blind Spot"), tr("Metrics")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, pathToggles, pathToggleNames);
    } else if (param == "PedalsOnUI") {
      std::vector<QString> pedalsToggles{"DynamicPedalsOnUI", "StaticPedalsOnUI"};
      std::vector<QString> pedalsToggleNames{tr("Dynamic"), tr("Static")};
      FrogPilotParamToggleControl *pedalsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, pedalsToggles, pedalsToggleNames);
      QObject::connect(pedalsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, pedalsToggle](int index) {
        if (index == 0) {
          params.putBool("StaticPedalsOnUI", false);
        } else if (index == 1) {
          params.putBool("DynamicPedalsOnUI", false);
        }

        pedalsToggle->updateButtonStates();
      });
      toggle = pedalsToggle;

    } else if (param == "WheelIcon") {
      std::vector<QString> wheelToggles{"RotatingWheel"};
      std::vector<QString> wheelToggleNames{"Live Rotation"};
      std::map<int, QString> steeringWheelLabels = {{-1, tr("None")}, {0, tr("Stock")}, {1, tr("Lexus")}, {2, tr("Toyota")}, {3, tr("Frog")}, {4, tr("Rocket")}, {5, tr("Hyundai")}, {6, tr("Stalin")}};
      toggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, -1, 6, steeringWheelLabels, this, true, "", 1, 1, wheelToggles, wheelToggleNames);

    } else if (param == "DeveloperUI") {
      FrogPilotParamManageControl *developerUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(developerUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedDeveloperUIKeys  = developerUIKeys ;

          toggle->setVisible(modifiedDeveloperUIKeys.find(key.c_str()) != modifiedDeveloperUIKeys.end());
        }
      });
      toggle = developerUIToggle;
    } else if (param == "BorderMetrics") {
      std::vector<QString> borderToggles{"BlindSpotMetrics", "ShowSteering", "SignalMetrics"};
      std::vector<QString> borderToggleNames{tr("Blind Spot"), tr("Steering Torque"), tr("Turn Signal"), };
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, borderToggles, borderToggleNames);
    } else if (param == "NumericalTemp") {
      std::vector<QString> temperatureToggles{"Fahrenheit"};
      std::vector<QString> temperatureToggleNames{tr("Fahrenheit")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, temperatureToggles, temperatureToggleNames);
    } else if (param == "SidebarMetrics") {
      std::vector<QString> sidebarMetricsToggles{"ShowCPU", "ShowGPU", "ShowIP", "ShowMemoryUsage", "ShowStorageLeft", "ShowStorageUsed"};
      std::vector<QString> sidebarMetricsToggleNames{tr("CPU"), tr("GPU"), tr("IP"), tr("RAM"), tr("SSD Left"), tr("SSD Used")};
      FrogPilotParamToggleControl *sidebarMetricsToggle = new FrogPilotParamToggleControl(param, title, desc, icon, sidebarMetricsToggles, sidebarMetricsToggleNames, this, 125);
      QObject::connect(sidebarMetricsToggle, &FrogPilotParamToggleControl::buttonTypeClicked, this, [this, sidebarMetricsToggle](int index) {
        if (index == 0) {
          params.putBool("ShowGPU", false);
        } else if (index == 1) {
          params.putBool("ShowCPU", false);
        } else if (index == 3) {
          params.putBool("ShowStorageLeft", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 4) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageUsed", false);
        } else if (index == 5) {
          params.putBool("ShowMemoryUsage", false);
          params.putBool("ShowStorageLeft", false);
        }

        sidebarMetricsToggle->updateButtonStates();
      });
      toggle = sidebarMetricsToggle;

    } else if (param == "ModelUI") {
      FrogPilotParamManageControl *modelUIToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelUIToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedModelUIKeysKeys = modelUIKeys;

          if (!hasOpenpilotLongitudinal) {
            modifiedModelUIKeysKeys.erase("HideLeadMarker");
          }

          toggle->setVisible(modifiedModelUIKeysKeys.find(key.c_str()) != modifiedModelUIKeysKeys.end());
        }
      });
      toggle = modelUIToggle;
    } else if (param == "LaneLinesWidth" || param == "RoadEdgesWidth") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 24, std::map<int, QString>(), this, false, tr(" inches"));
    } else if (param == "PathEdgeWidth") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr("%"));
    } else if (param == "PathWidth") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, tr(" feet"), 10);

    } else if (param == "QOLVisuals") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(qolKeys.find(key.c_str()) != qolKeys.end());
        }
      });
      toggle = qolToggle;
    } else if (param == "CameraView") {
      std::vector<QString> cameraOptions{tr("Auto"), tr("Driver"), tr("Standard"), tr("Wide")};
      FrogPilotButtonParamControl *preferredCamera = new FrogPilotButtonParamControl(param, title, desc, icon, cameraOptions);
      toggle = preferredCamera;
    } else if (param == "BigMap") {
      std::vector<QString> mapToggles{"FullMap"};
      std::vector<QString> mapToggleNames{tr("Full Map")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, mapToggles, mapToggleNames);
    } else if (param == "HideSpeed") {
      std::vector<QString> hideSpeedToggles{"HideSpeedUI"};
      std::vector<QString> hideSpeedToggleNames{tr("Control Via UI")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, hideSpeedToggles, hideSpeedToggleNames);
    } else if (param == "MapStyle") {
      QMap<int, QString> styleMap = {
        {0, tr("Stock openpilot")},
        {1, tr("Mapbox Streets")},
        {2, tr("Mapbox Outdoors")},
        {3, tr("Mapbox Light")},
        {4, tr("Mapbox Dark")},
        {5, tr("Mapbox Satellite")},
        {6, tr("Mapbox Satellite Streets")},
        {7, tr("Mapbox Navigation Day")},
        {8, tr("Mapbox Navigation Night")},
        {9, tr("Mapbox Traffic Night")},
        {10, tr("mike854's (Satellite hybrid)")},
      };

      QStringList styles = styleMap.values();
      ButtonControl *mapStyleButton = new ButtonControl(title, tr("SELECT"), desc);
      QObject::connect(mapStyleButton, &ButtonControl::clicked, [=]() {
        QStringList styles = styleMap.values();
        QString selection = MultiOptionDialog::getSelection(tr("Select a map style"), styles, "", this);
        if (!selection.isEmpty()) {
          int selectedStyle = styleMap.key(selection);
          params.putIntNonBlocking("MapStyle", selectedStyle);
          mapStyleButton->setValue(selection);
          updateFrogPilotToggles();
        }
      });

      int currentStyle = params.getInt("MapStyle");
      mapStyleButton->setValue(styleMap[currentStyle]);

      toggle = mapStyleButton;

    } else if (param == "ScreenManagement") {
      FrogPilotParamManageControl *screenToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(screenToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(screenKeys.find(key.c_str()) != screenKeys.end());
        }
      });
      toggle = screenToggle;
    } else if (param == "HideUIElements") {
      std::vector<QString> uiElementsToggles{"HideAlerts", "HideMapIcon", "HideMaxSpeed"};
      std::vector<QString> uiElementsToggleNames{tr("Alerts"), tr("Map Icon"), tr("Max Speed")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, uiElementsToggles, uiElementsToggleNames);
    } else if (param == "ScreenBrightness" || param == "ScreenBrightnessOnroad") {
      std::map<int, QString> brightnessLabels;
      if (param == "ScreenBrightnessOnroad") {
        for (int i = 0; i <= 101; i++) {
          brightnessLabels[i] = (i == 0) ? tr("Screen Off") : (i == 101) ? tr("Auto") : QString::number(i) + "%";
        }
        toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 101, brightnessLabels, this, false);
      } else {
        for (int i = 1; i <= 101; i++) {
          brightnessLabels[i] = (i == 101) ? tr("Auto") : QString::number(i) + "%";
        }
        toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 101, brightnessLabels, this, false);
      }
    } else if (param == "ScreenTimeout" || param == "ScreenTimeoutOnroad") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 5, 60, std::map<int, QString>(), this, false, tr(" seconds"));

    } else {
      toggle = new ParamControl(param, title, desc, icon, this);
    }

    addItem(toggle);
    toggles[param.toStdString()] = toggle;

    QObject::connect(static_cast<ToggleControl*>(toggle), &ToggleControl::toggleFlipped, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamValueControl*>(toggle), &FrogPilotParamValueControl::valueChanged, &updateFrogPilotToggles);

    QObject::connect(toggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });

    QObject::connect(static_cast<FrogPilotParamManageControl*>(toggle), &FrogPilotParamManageControl::manageButtonClicked, [this]() {
      update();
    });
  }

  QObject::connect(parent, &SettingsWindow::closeParentToggle, this, &FrogPilotVisualsPanel::hideToggles);
  QObject::connect(parent, &SettingsWindow::updateMetric, this, &FrogPilotVisualsPanel::updateMetric);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &FrogPilotVisualsPanel::updateCarToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotVisualsPanel::updateState);

  updateMetric();
}

void FrogPilotVisualsPanel::showEvent(QShowEvent *event) {
  hasOpenpilotLongitudinal = hasOpenpilotLongitudinal && !params.getBool("DisableOpenpilotLongitudinal");
}

void FrogPilotVisualsPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  started = s.scene.started;
}

void FrogPilotVisualsPanel::updateCarToggles() {
  auto carParams = params.get("CarParamsPersistent");
  if (!carParams.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(carParams.data(), carParams.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();
    auto carName = CP.getCarName();

    hasAutoTune = (carName == "hyundai" || carName == "toyota") && CP.getLateralTuning().which() == cereal::CarParams::LateralTuning::TORQUE;
    hasBSM = CP.getEnableBsm();
    hasOpenpilotLongitudinal = CP.getOpenpilotLongitudinalControl() && !params.getBool("DisableOpenpilotLongitudinal");
  } else {
    hasBSM = true;
    hasOpenpilotLongitudinal = true;
  }

  hideToggles();
}

void FrogPilotVisualsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? INCH_TO_CM : CM_TO_INCH;
    double speedConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;
    params.putIntNonBlocking("LaneLinesWidth", std::nearbyint(params.getInt("LaneLinesWidth") * distanceConversion));
    params.putIntNonBlocking("RoadEdgesWidth", std::nearbyint(params.getInt("RoadEdgesWidth") * distanceConversion));
    params.putIntNonBlocking("PathWidth", std::nearbyint(params.getInt("PathWidth") * speedConversion));
  }

  FrogPilotParamValueControl *laneLinesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneLinesWidth"]);
  FrogPilotParamValueControl *roadEdgesWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["RoadEdgesWidth"]);
  FrogPilotParamValueControl *pathWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["PathWidth"]);

  if (isMetric) {
    laneLinesWidthToggle->setDescription(tr("Customize the lane line width.\n\nDefault matches the Vienna average of 10 centimeters."));
    roadEdgesWidthToggle->setDescription(tr("Customize the road edges width.\n\nDefault is 1/2 of the Vienna average lane line width of 10 centimeters."));

    laneLinesWidthToggle->updateControl(0, 60, tr(" centimeters"));
    roadEdgesWidthToggle->updateControl(0, 60, tr(" centimeters"));
    pathWidthToggle->updateControl(0, 30, tr(" meters"), 10);
  } else {
    laneLinesWidthToggle->setDescription(tr("Customize the lane line width.\n\nDefault matches the MUTCD average of 4 inches."));
    roadEdgesWidthToggle->setDescription(tr("Customize the road edges width.\n\nDefault is 1/2 of the MUTCD average lane line width of 4 inches."));

    laneLinesWidthToggle->updateControl(0, 24, tr(" inches"));
    roadEdgesWidthToggle->updateControl(0, 24, tr(" inches"));
    pathWidthToggle->updateControl(0, 100, tr(" feet"), 10);
  }

  laneLinesWidthToggle->refresh();
  roadEdgesWidthToggle->refresh();
}

void FrogPilotVisualsPanel::hideToggles() {
  for (auto &[key, toggle] : toggles) {
    bool subToggles = alertVolumeControlKeys.find(key.c_str()) != alertVolumeControlKeys.end() ||
                      customAlertsKeys.find(key.c_str()) != customAlertsKeys.end() ||
                      customOnroadUIKeys.find(key.c_str()) != customOnroadUIKeys.end() ||
                      customThemeKeys.find(key.c_str()) != customThemeKeys.end() ||
                      developerUIKeys.find(key.c_str()) != developerUIKeys.end() ||
                      modelUIKeys.find(key.c_str()) != modelUIKeys.end() ||
                      qolKeys.find(key.c_str()) != qolKeys.end() ||
                      screenKeys.find(key.c_str()) != screenKeys.end();
    toggle->setVisible(!subToggles);
  }

  update();
}
