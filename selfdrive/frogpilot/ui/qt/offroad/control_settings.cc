#include <filesystem>
#include <iostream>

#include "selfdrive/frogpilot/ui/qt/offroad/control_settings.h"

namespace fs = std::filesystem;

bool checkCommaNNFFSupport(const std::string &carFingerprint) {
  const std::string filePath = "../car/torque_data/neural_ff_weights.json";

  if (!std::filesystem::exists(filePath)) {
    return false;
  }

  std::ifstream file(filePath);
  std::string line;
  while (std::getline(file, line)) {
    if (line.find(carFingerprint) != std::string::npos) {
      std::cout << "comma's NNFF supports fingerprint: " << carFingerprint << std::endl;
      return true;
    }
  }

  return false;
}

bool checkNNFFLogFileExists(const std::string &carFingerprint) {
  const fs::path dirPath("../car/torque_data/lat_models");

  if (!fs::exists(dirPath)) {
    std::cerr << "Directory does not exist: " << fs::absolute(dirPath) << std::endl;
    return false;
  }

  for (const auto &entry : fs::directory_iterator(dirPath)) {
    if (entry.path().filename().string().find(carFingerprint) == 0) {
      std::cout << "NNFF supports fingerprint: " << entry.path().filename() << std::endl;
      return true;
    }
  }

  return false;
}

FrogPilotControlsPanel::FrogPilotControlsPanel(SettingsWindow *parent) : FrogPilotListWidget(parent) {
  std::string branch = params.get("GitBranch");
  isRelease = branch == "FrogPilot";
  const std::vector<std::tuple<QString, QString, QString, QString>> controlToggles {
    {"AlwaysOnLateral", tr("始终开启横向控制"), tr("在使用刹车或油门踏板时保持openpilot的横向控制。\n\n仅通过'巡航控制'按钮停用。"), "../frogpilot/assets/toggle_icons/icon_always_on_lateral.png"},
    {"AlwaysOnLateralMain", tr("启用巡航主控"), tr("通过点击'巡航控制'按钮启用'Always On Lateral'，无需先启用openpilot。"), ""},
    {"PauseAOLOnBrake", tr("刹车时暂停"), tr("当刹车踏板在设定速度以下被按下时暂停'Always On Lateral'。"), ""},
    {"HideAOLStatusBar", tr("隐藏状态栏"), tr("不使用'Always On Lateral'的状态栏。"), ""},

    {"ConditionalExperimental", tr("条件实验模式"), tr("在预定义条件下自动切换到'实验模式'。"), "../frogpilot/assets/toggle_icons/icon_conditional.png"},
    {"CECurves", tr("检测到曲线"), tr("检测到曲线时切换到'实验模式'。"), ""},
    {"CENavigation", tr("基于导航"), tr("基于导航数据切换到'实验模式'。（例如：交叉路口、停车标志、即将转弯等）"), ""},
    {"CESlowerLead", tr("检测到较慢/停止的前车"), tr("检测到前方有较慢或停止的前车时切换到'实验模式'。"), ""},
    {"CEStopLights", tr("红绿灯和停车标志"), tr("检测到红绿灯或停车标志时切换到'实验模式'。"), ""},
    {"CESignal", tr("低速时使用转向灯"), tr("在高速公路速度以下使用转向灯时切换到'实验模式'以帮助转弯。"), ""},
    {"HideCEMStatusBar", tr("隐藏状态栏"), tr("不使用'条件实验模式'的状态栏。"), ""},

    {"DeviceManagement", tr("设备管理"), tr("根据个人喜好调整设备的行为。"), "../frogpilot/assets/toggle_icons/icon_device.png"},
    {"DeviceShutdown", tr("设备关闭计时器"), tr("配置设备在离开道路后多快关闭。"), ""},
    {"NoLogging", tr("禁用日志记录"), tr("关闭所有数据跟踪以增强隐私或减少热负荷。"), ""},
    {"NoUploads", tr("禁用上传"), tr("关闭所有数据上传到comma的服务器。"), ""},
    {"IncreaseThermalLimits", tr("提高热安全限制"), tr("允许设备在高于comma推荐的热限温度下运行。"), ""},
    {"LowVoltageShutdown", tr("低电压关闭阈值"), tr("当电池达到特定电压水平时自动关闭设备以防止电池损坏。"), ""},
    {"OfflineMode", tr("离线模式"), tr("允许设备无限期离线。"), ""},

    {"DrivingPersonalities", tr("驾驶个性化"), tr("管理comma的'个性化配置文件'的驾驶行为。"), "../frogpilot/assets/toggle_icons/icon_personality.png"},
    {"CustomPersonalities", tr("自定义个性化"), tr("根据您的驾驶风格自定义驾驶个性化配置文件。"), ""},
    {"TrafficPersonalityProfile", tr("交通个性化"), tr("自定义'Traffic'个性化配置文件。"), "../frogpilot/assets/other_images/traffic.png"},
    {"TrafficFollow", tr("跟车距离"), tr("设置使用'Traffic Mode'时的最小跟车距离。您的跟车距离将在0到%1之间动态调整，介于此距离和'Aggressive'配置文件的跟车距离之间。\n\n例如：\n\nTraffic Mode: 0.5秒\nAggressive: 1.0秒\n\n0%2 = 0.5秒\n%3 = 0.75秒\n%1 = 1.0秒"), ""},
    {"TrafficJerkAcceleration", tr("加速/减速响应偏移"), tr("自定义使用'Traffic Mode'时的加速响应率。"), ""},
    {"TrafficJerkSpeed", tr("速度控制响应偏移"), tr("自定义使用'Traffic Mode'时保持速度（包括制动）的响应率。"), ""},
    {"ResetTrafficPersonality", tr("重置设置"), tr("将'Traffic Mode'个性化配置的值重置为默认值。"), ""},
    {"AggressivePersonalityProfile", tr("激进个性化"), tr("自定义'Aggressive'个性化配置文件。"), "../frogpilot/assets/other_images/aggressive.png"},
    {"AggressiveFollow", tr("跟车距离"), tr("设置'Aggressive'个性化配置的跟车距离。表示跟随前车的秒数。\n\n默认值：1.25秒。"), ""},
    {"AggressiveJerkAcceleration", tr("加速/减速响应偏移"), tr("自定义使用'Aggressive'个性化配置时的加速响应率。"), ""},
    {"AggressiveJerkSpeed", tr("速度控制响应偏移"), tr("自定义使用'Aggressive'个性化配置时保持速度（包括制动）的响应率。"), ""},
    {"ResetAggressivePersonality", tr("重置设置"), tr("将'Aggressive'个性化配置的值重置为默认值。"), ""},
    {"StandardPersonalityProfile", tr("标准个性化"), tr("自定义'Standard'个性化配置文件。"), "../frogpilot/assets/other_images/standard.png"},
    {"StandardFollow", tr("跟车距离"), tr("设置'Standard'个性化配置的跟车距离。表示跟随前车的秒数。\n\n默认值：1.45秒。"), ""},
    {"StandardJerkAcceleration", tr("加速/减速响应偏移"), tr("自定义使用'Standard'个性化配置时的加速响应率。"), ""},
    {"StandardJerkSpeed", tr("速度控制响应偏移"), tr("自定义使用'Standard'个性化配置时保持速度（包括制动）的响应率。"), ""},
    {"ResetStandardPersonality", tr("重置设置"), tr("将'Standard'个性化配置的值重置为默认值。"), ""},
    {"RelaxedPersonalityProfile", tr("放松个性化"), tr("自定义'Relaxed'个性化配置文件。"), "../frogpilot/assets/other_images/relaxed.png"},
    {"RelaxedFollow", tr("跟车距离"), tr("设置'Relaxed'个性化配置的跟车距离。表示跟随前车的秒数。\n\n默认值：1.75秒。"), ""},
    {"RelaxedJerkAcceleration", tr("加速/减速响应偏移"), tr("自定义使用'Relaxed'个性化配置时的加速响应率。"), ""},
    {"RelaxedJerkSpeed", tr("速度控制响应偏移"), tr("自定义使用'Relaxed'个性化配置时保持速度（包括制动）的响应率。"), ""},
    {"ResetRelaxedPersonality", tr("重置设置"), tr("将'Relaxed'个性化配置的值重置为默认值。"), ""},
    {"OnroadDistanceButton", tr("道路距离按钮"), tr("通过onroad UI模拟一个距离按钮来控制个性化配置、'实验模式'和'Traffic Mode'。"), ""},
    {"ExperimentalModeActivation", tr("实验模式激活"), tr("通过方向盘或屏幕上的按钮切换实验模式。\n\n覆盖'条件实验模式'。"), "../assets/img_experimental_white.svg"},
    {"ExperimentalModeViaLKAS", tr("双击LKAS"), tr("通过双击方向盘上的'LKAS'按钮启用/禁用'实验模式'。"), ""},
    {"ExperimentalModeViaTap", tr("双击UI"), tr("通过在0.5秒内双击onroad UI启用/禁用'实验模式'。"), ""},
    {"ExperimentalModeViaDistance", tr("长按距离"), tr("通过在方向盘上长按'distance'按钮0.5秒启用/禁用'实验模式'。"), ""},

    {"LaneChangeCustomizations", tr("变道自定义"), tr("自定义openpilot中的变道行为。"), "../frogpilot/assets/toggle_icons/icon_lane.png"},
    {"MinimumLaneChangeSpeed", tr("最小变道速度"), tr("自定义允许openpilot变道的最小驾驶速度。"), ""},
    {"NudgelessLaneChange", tr("无推挤变道"), tr("启用无需手动转向输入的变道。"), ""},
    {"LaneChangeTime", tr("变道计时器"), tr("设置执行变道前的延迟。"), ""},
    {"LaneDetectionWidth", tr("车道检测阈值"), tr("设置被视为车道所需的车道宽度。"), ""},
    {"OneLaneChange", tr("每个信号一个变道"), tr("每次转向信号激活只允许一次变道。"), ""},

    {"LateralTune", tr("横向调节"), tr("修改openpilot的转向行为。"), "../frogpilot/assets/toggle_icons/icon_lateral_tune.png"},
    {"ForceAutoTune", tr("强制自动调节"), tr("强制comma的自动横向调节以支持不受支持的车辆。"), ""},
    {"NNFF", tr("NNFF"), tr("使用Twilsonco的神经网络前馈以增强横向控制的精度。"), ""},
    {"NNFFLite", tr("NNFF-Lite"), tr("为没有可用NNFF日志的车辆使用Twilsonco的神经网络前馈以增强横向控制的精度。"), ""},
    {"SteerRatio", steerRatioStock != 0 ? QString(tr("转向比（默认值：%1）")).arg(QString::number(steerRatioStock, 'f', 2)) : tr("转向比"), tr("使用自定义转向比，而不是comma的自动调节值。"), ""},
    {"TacoTune", tr("Taco调节"), tr("使用comma的'Taco调节'，旨在处理左转和右转。"), ""},
    {"TurnDesires", tr("使用转向期望"), tr("在低于最小变道速度的转弯中使用转向期望以提高精度。"), ""},

    {"LongitudinalTune", tr("纵向调节"), tr("修改openpilot的加速和制动行为。"), "../frogpilot/assets/toggle_icons/icon_longitudinal_tune.png"},
    {"AccelerationProfile", tr("加速配置文件"), tr("将加速率更改为运动型或环保型。"), ""},
    {"DecelerationProfile", tr("减速配置文件"), tr("将减速率更改为运动型或环保型。"), ""},
    {"AggressiveAcceleration", tr("增加跟随前车的加速"), tr("在跟随更快的前车时增加攻击性。"), ""},
    {"StoppingDistance", tr("增加跟随前车的停止距离"), tr("增加停止距离，以便更舒适地停靠在前车后。"), ""},
    {"LeadDetectionThreshold", tr("前车检测阈值"), tr("增加或减少前车检测阈值，以便更早检测前车或提高模型信心。"), ""},
    {"SmoothBraking", tr("更平稳的制动"), tr("在接近较慢的车辆时平滑制动行为。"), ""},
    {"TrafficMode", tr("交通模式"), tr("通过长按'distance'按钮2.5秒启用'交通模式'。当'交通模式'处于活动状态时，onroad UI将变为红色，openpilot将针对停走交通进行驾驶。"), ""},

    {"MTSCEnabled", tr("地图转弯速度控制"), tr("为下载地图检测到的预期曲线减速。"), "../frogpilot/assets/toggle_icons/icon_speed_map.png"},
    {"DisableMTSCSmoothing", tr("禁用MTSC UI平滑"), tr("禁用onroad UI中请求速度的平滑，以准确显示MTSC当前请求的速度。"), ""},
    {"MTSCCurvatureCheck",  tr("模型曲率检测故障安全"), tr("仅在模型检测到道路曲线时触发MTSC。纯粹用作故障安全，以防止误报。如果您从未遇到误报，请将其关闭。"), ""},
    {"MTSCAggressiveness", tr("转弯速度攻击性"), tr("设置转弯速度的攻击性。更高的值会导致更快的转弯，较低的值会导致更温和的转弯。\n\n+-1%的变化会导致速度大约提高或降低1 mph。"), ""},

    {"ModelSelector", tr("模型选择器"), tr("管理openpilot的驾驶模型。"), "../assets/offroad/icon_calibration.png"},
    {"QOLControls", tr("生活质量"), tr("各种生活质量的改进，以提升您整体的openpilot体验。"), "../frogpilot/assets/toggle_icons/quality_of_life.png"},
    {"CustomCruise", tr("巡航加速间隔"), tr("设置一个自定义间隔来增加最大设定速度。"), ""},
    {"CustomCruiseLong", tr("巡航加速间隔（长按）"), tr("在按住巡航加速按钮时设置一个自定义间隔来增加最大设定速度。"), ""},
    {"MapGears", tr("将加速/减速映射到档位"), tr("将您的加速/减速配置映射到您的'Eco'和/或'Sport'档位。"), ""},
    {"PauseLateralSpeed", tr("低于设定速度暂停横向控制"), tr("在所有低于设定速度的速度下暂停横向控制。"), ""},
    {"ReverseCruise", tr("反向巡航加速"), tr("反转'长按'功能逻辑，以将最大设定速度增加5而不是1。适合快速增加最大速度。"), ""},
    {"SetSpeedOffset", tr("设置速度偏移"), tr("为您期望的设定速度设置一个偏移。"), ""},

    {"SpeedLimitController", tr("速度限制控制器"), tr("使用'开放街道地图'、'在openpilot上导航'或您汽车的仪表盘（仅限丰田/雷克萨斯/HKG）自动调整最大速度以匹配当前速度限制。"), "../assets/offroad/icon_speed_limit.png"},
    {"SLCControls", tr("控制设置"), tr("管理与'速度限制控制器'相关的切换。"), ""},
    {"Offset1", tr("速度限制偏移（0-34 mph）"), tr("适用于速度限制在0-34 mph之间的速度限制偏移。"), ""},
    {"Offset2", tr("速度限制偏移（35-54 mph）"), tr("适用于速度限制在35-54 mph之间的速度限制偏移。"), ""},
    {"Offset3", tr("速度限制偏移（55-64 mph）"), tr("适用于速度限制在55-64 mph之间的速度限制偏移。"), ""},
    {"Offset4", tr("速度限制偏移（65-99 mph）"), tr("适用于速度限制在65-99 mph之间的速度限制偏移。"), ""},
    {"SLCFallback", tr("回退方法"), tr("选择在没有可用速度限制时的回退方法。"), ""},
    {"SLCOverride", tr("覆盖方法"), tr("选择您首选的覆盖当前速度限制的方法。"), ""},
    {"SLCPriority", tr("优先顺序"), tr("配置速度限制的优先顺序。"), ""},
    {"SLCQOL", tr("生活质量设置"), tr("管理与'速度限制控制器'的生活质量功能相关的切换。"), ""},
    {"SLCConfirmation", tr("确认新速度限制"), tr("在手动确认之前，不要自动开始使用新的速度限制。"), ""},
    {"ForceMPHDashboard", tr("强制从仪表盘读取MPH"), tr("强制从仪表盘读取MPH。仅在您居住的地区仪表盘的速度限制为KPH，但您使用MPH时使用此功能。"), ""},
    {"SLCLookaheadHigher", tr("为更高的速度限制做准备"), tr("设置一个'前瞻'值，以准备即将到来的高于您当前速度限制的速度限制，使用存储在'开放街道地图'中的数据。"), ""},
    {"SLCLookaheadLower", tr("为更低的速度限制做准备"), tr("设置一个'前瞻'值，以准备即将到来的低于您当前速度限制的速度限制，使用存储在'开放街道地图'中的数据。"), ""},
    {"SetSpeedLimit", tr("将当前速度限制用作设定速度"), tr("如果在您最初启用openpilot时有一个速度限制，则将您的最大速度设置为当前速度限制。"), ""},
    {"SLCVisuals", tr("视觉设置"), tr("管理与'速度限制控制器'的视觉相关的切换。"), ""},
    {"ShowSLCOffset", tr("显示速度限制偏移"), tr("在使用'速度限制控制器'时，在onroad UI中显示与速度限制分开的速度限制偏移。"), ""},
    {"SpeedLimitChangedAlert", tr("速度限制变更警报"), tr("每当速度限制变化时触发警报。"), ""},
    {"UseVienna", tr("使用维也纳速度限制标志"), tr("使用维也纳（欧盟）速度限制样式标志，而不是MUTCD（美国）。"), ""},

    {"VisionTurnControl", tr("视觉转向速度控制器"), tr("在检测到道路曲线时减速。"), "../frogpilot/assets/toggle_icons/icon_vtc.png"},
    {"DisableVTSCSmoothing", tr("禁用VTSC UI平滑"), tr("禁用onroad UI中请求速度的平滑。"), ""},
    {"CurveSensitivity", tr("曲线检测灵敏度"), tr("设置曲线检测灵敏度。更高的值会促使更早的响应，较低的值会导致更平滑但较晚的反应。"), ""},
    {"TurnAggressiveness", tr("转向速度攻击性"), tr("设置转向速度的攻击性。更高的值会导致更快的转弯，较低的值会导致更温和的转弯。"), ""},
  };

  for (const auto &[param, title, desc, icon] : controlToggles) {
    AbstractControl *toggle;

    if (param == "AlwaysOnLateral") {
      FrogPilotParamManageControl *aolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(aolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(aolKeys.find(key.c_str()) != aolKeys.end());
        }
      });
      toggle = aolToggle;
    } else if (param == "PauseAOLOnBrake") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr(" mph"));

    } else if (param == "ConditionalExperimental") {
      FrogPilotParamManageControl *conditionalExperimentalToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(conditionalExperimentalToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        conditionalSpeedsImperial->setVisible(!isMetric);
        conditionalSpeedsMetric->setVisible(isMetric);
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(conditionalExperimentalKeys.find(key.c_str()) != conditionalExperimentalKeys.end());
        }
      });
      toggle = conditionalExperimentalToggle;
    } else if (param == "CECurves") {
      FrogPilotParamValueControl *CESpeedImperial = new FrogPilotParamValueControl("CESpeed", tr("Below"), tr("Switch to 'Experimental Mode' below this speed when not following a lead vehicle."), "", 0, 99,
                                                                                   std::map<int, QString>(), this, false, tr(" mph"));
      FrogPilotParamValueControl *CESpeedLeadImperial = new FrogPilotParamValueControl("CESpeedLead", tr("  w/Lead"), tr("Switch to 'Experimental Mode' below this speed when following a lead vehicle."), "", 0, 99,
                                                                                       std::map<int, QString>(), this, false, tr(" mph"));
      conditionalSpeedsImperial = new FrogPilotDualParamControl(CESpeedImperial, CESpeedLeadImperial, this);
      addItem(conditionalSpeedsImperial);

      FrogPilotParamValueControl *CESpeedMetric = new FrogPilotParamValueControl("CESpeed", tr("Below"), tr("Switch to 'Experimental Mode' below this speed in absence of a lead vehicle."), "", 0, 150,
                                                                                 std::map<int, QString>(), this, false, tr(" kph"));
      FrogPilotParamValueControl *CESpeedLeadMetric = new FrogPilotParamValueControl("CESpeedLead", tr("  w/Lead"), tr("Switch to 'Experimental Mode' below this speed when following a lead vehicle."), "", 0, 150,
                                                                                     std::map<int, QString>(), this, false, tr(" kph"));
      conditionalSpeedsMetric = new FrogPilotDualParamControl(CESpeedMetric, CESpeedLeadMetric, this);
      addItem(conditionalSpeedsMetric);

      std::vector<QString> curveToggles{"CECurvesLead"};
      std::vector<QString> curveToggleNames{tr("With Lead")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, curveToggles, curveToggleNames);
    } else if (param == "CENavigation") {
      std::vector<QString> navigationToggles{"CENavigationIntersections", "CENavigationTurns", "CENavigationLead"};
      std::vector<QString> navigationToggleNames{tr("Intersections"), tr("Turns"), tr("With Lead")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, navigationToggles, navigationToggleNames);
    } else if (param == "CEStopLights") {
      std::vector<QString> stopLightToggles{"CEStopLightsLead"};
      std::vector<QString> stopLightToggleNames{tr("With Lead")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, stopLightToggles, stopLightToggleNames);

    } else if (param == "DeviceManagement") {
      FrogPilotParamManageControl *deviceManagementToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(deviceManagementToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(deviceManagementKeys.find(key.c_str()) != deviceManagementKeys.end());
        }
      });
      toggle = deviceManagementToggle;
    } else if (param == "DeviceShutdown") {
      std::map<int, QString> shutdownLabels;
      for (int i = 0; i <= 33; ++i) {
        shutdownLabels[i] = i == 0 ? tr("5 mins") : i <= 3 ? QString::number(i * 15) + tr(" mins") : QString::number(i - 3) + (i == 4 ? tr(" hour") : tr(" hours"));
      }
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 33, shutdownLabels, this, false);
    } else if (param == "NoUploads") {
      std::vector<QString> uploadsToggles{"DisableOnroadUploads"};
      std::vector<QString> uploadsToggleNames{tr("Only Onroad")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, uploadsToggles, uploadsToggleNames);
    } else if (param == "LowVoltageShutdown") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 11.8, 12.5, std::map<int, QString>(), this, false, tr(" volts"), 1, 0.01);

    } else if (param == "DrivingPersonalities") {
      FrogPilotParamManageControl *drivingPersonalitiesToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(drivingPersonalitiesToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end());
        }
      });
      toggle = drivingPersonalitiesToggle;
    } else if (param == "CustomPersonalities") {
      FrogPilotParamManageControl *customPersonalitiesToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(customPersonalitiesToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        customPersonalitiesOpen = true;
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end());
          openSubParentToggle();
        }
      });

      personalitiesInfoBtn = new ButtonControl(tr("What Do All These Do?"), tr("VIEW"), tr("Learn what all the values in 'Custom Personality Profiles' do on openpilot's driving behaviors."));
      connect(personalitiesInfoBtn, &ButtonControl::clicked, [=]() {
        const std::string txt = util::read_file("../frogpilot/ui/qt/offroad/personalities_info.txt");
        ConfirmationDialog::rich(QString::fromStdString(txt), this);
      });
      addItem(personalitiesInfoBtn);

      toggle = customPersonalitiesToggle;
    } else if (param == "ResetTrafficPersonality" || param == "ResetAggressivePersonality" || param == "ResetStandardPersonality" || param == "ResetRelaxedPersonality") {
      std::vector<QString> personalityOptions{tr("Reset")};
      FrogPilotButtonsControl *profileBtn = new FrogPilotButtonsControl(title, desc, icon, personalityOptions);
      toggle = profileBtn;
    } else if (param == "TrafficPersonalityProfile") {
      FrogPilotParamManageControl *trafficPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(trafficPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(trafficPersonalityKeys.find(key.c_str()) != trafficPersonalityKeys.end());
        }
        openSubSubParentToggle();
        personalitiesInfoBtn->setVisible(true);
      });
      toggle = trafficPersonalityToggle;
    } else if (param == "AggressivePersonalityProfile") {
      FrogPilotParamManageControl *aggressivePersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(aggressivePersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(aggressivePersonalityKeys.find(key.c_str()) != aggressivePersonalityKeys.end());
        }
        openSubSubParentToggle();
        personalitiesInfoBtn->setVisible(true);
      });
      toggle = aggressivePersonalityToggle;
    } else if (param == "StandardPersonalityProfile") {
      FrogPilotParamManageControl *standardPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(standardPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(standardPersonalityKeys.find(key.c_str()) != standardPersonalityKeys.end());
        }
        openSubSubParentToggle();
        personalitiesInfoBtn->setVisible(true);
      });
      toggle = standardPersonalityToggle;
    } else if (param == "RelaxedPersonalityProfile") {
      FrogPilotParamManageControl *relaxedPersonalityToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(relaxedPersonalityToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end());
        }
        openSubSubParentToggle();
        personalitiesInfoBtn->setVisible(true);
      });
      toggle = relaxedPersonalityToggle;
    } else if (trafficPersonalityKeys.find(param) != trafficPersonalityKeys.end() ||
               aggressivePersonalityKeys.find(param) != aggressivePersonalityKeys.end() ||
               standardPersonalityKeys.find(param) != standardPersonalityKeys.end() ||
               relaxedPersonalityKeys.find(param) != relaxedPersonalityKeys.end()) {
      if (param == "TrafficFollow" || param == "AggressiveFollow" || param == "StandardFollow" || param == "RelaxedFollow") {
        if (param == "TrafficFollow") {
          toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0.5, 5, std::map<int, QString>(), this, false, tr(" seconds"), 1, 0.01);
        } else {
          toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 5, std::map<int, QString>(), this, false, tr(" seconds"), 1, 0.01);
        }
      } else {
        toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 500, std::map<int, QString>(), this, false, "%");
      }
    } else if (param == "OnroadDistanceButton") {
      std::vector<QString> onroadDistanceToggles{"KaofuiIcons"};
      std::vector<QString> onroadDistanceToggleNames{tr("Kaofui's Icons")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, onroadDistanceToggles, onroadDistanceToggleNames);

    } else if (param == "ExperimentalModeActivation") {
      FrogPilotParamManageControl *experimentalModeActivationToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(experimentalModeActivationToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(experimentalModeActivationKeys.find(key.c_str()) != experimentalModeActivationKeys.end());
        }
      });
      toggle = experimentalModeActivationToggle;

    } else if (param == "LateralTune") {
      FrogPilotParamManageControl *lateralTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(lateralTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedLateralTuneKeys = lateralTuneKeys;

          if (hasAutoTune || params.getBool("LateralTune") && params.getBool("NNFF")) {
            modifiedLateralTuneKeys.erase("ForceAutoTune");
          }

          if (hasCommaNNFFSupport) {
            modifiedLateralTuneKeys.erase("NNFF");
            modifiedLateralTuneKeys.erase("NNFFLite");
          } else if (hasNNFFLog) {
            modifiedLateralTuneKeys.erase("NNFFLite");
          } else {
            modifiedLateralTuneKeys.erase("NNFF");
          }

          toggle->setVisible(modifiedLateralTuneKeys.find(key.c_str()) != modifiedLateralTuneKeys.end());
        }
      });
      toggle = lateralTuneToggle;
    } else if (param == "SteerRatio") {
      std::vector<QString> steerRatioToggles{"ResetSteerRatio"};
      std::vector<QString> steerRatioToggleNames{"Reset"};
      toggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, steerRatioStock * 0.75, steerRatioStock * 1.25, std::map<int, QString>(), this, false, "", 1, 0.01, steerRatioToggles, steerRatioToggleNames);

    } else if (param == "LongitudinalTune") {
      FrogPilotParamManageControl *longitudinalTuneToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(longitudinalTuneToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedLongitudinalTuneKeys = longitudinalTuneKeys;

          if (params.get("Model") == "radical-turtle") {
            modifiedLongitudinalTuneKeys.erase("LeadDetectionThreshold");
          }

          toggle->setVisible(modifiedLongitudinalTuneKeys.find(key.c_str()) != modifiedLongitudinalTuneKeys.end());
        }
      });
      toggle = longitudinalTuneToggle;
    } else if (param == "AccelerationProfile") {
      std::vector<QString> profileOptions{tr("Standard"), tr("Eco"), tr("Sport"), tr("Sport+")};
      FrogPilotButtonParamControl *profileSelection = new FrogPilotButtonParamControl(param, title, desc, icon, profileOptions);
      toggle = profileSelection;

      QObject::connect(static_cast<FrogPilotButtonParamControl*>(toggle), &FrogPilotButtonParamControl::buttonClicked, [this](int id) {
        if (id == 3) {
          FrogPilotConfirmationDialog::toggleAlert(tr("WARNING: This maxes out openpilot's acceleration from 2.0 m/s to 4.0 m/s and may cause oscillations when accelerating!"),
          tr("I understand the risks."), this);
        }
      });
    } else if (param == "AggressiveAcceleration") {
      std::vector<QString> accelerationToggles{"AggressiveAccelerationExperimental"};
      std::vector<QString> accelerationToggleNames{tr("Experimental")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, accelerationToggles, accelerationToggleNames);
      QObject::connect(static_cast<FrogPilotParamToggleControl*>(toggle), &FrogPilotParamToggleControl::buttonClicked, [this](bool checked) {
        if (checked) {
          FrogPilotConfirmationDialog::toggleAlert(
          tr("WARNING: This is very experimental and may cause the car to not brake or stop safely! Please report any issues in the FrogPilot Discord!"),
          tr("I understand the risks."), this);
        }
      });
    } else if (param == "DecelerationProfile") {
      std::vector<QString> profileOptions{tr("Standard"), tr("Eco"), tr("Sport")};
      FrogPilotButtonParamControl *profileSelection = new FrogPilotButtonParamControl(param, title, desc, icon, profileOptions);
      toggle = profileSelection;
    } else if (param == "StoppingDistance") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, std::map<int, QString>(), this, false, tr(" feet"));
    } else if (param == "LeadDetectionThreshold") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, "%");
    } else if (param == "SmoothBraking") {
      std::vector<QString> brakingToggles{"SmoothBrakingJerk", "SmoothBrakingFarLead"};
      std::vector<QString> brakingToggleNames{tr("Apply to Jerk"), tr("Far Lead Offset")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, brakingToggles, brakingToggleNames);
      QObject::connect(static_cast<FrogPilotParamToggleControl*>(toggle), &FrogPilotParamToggleControl::buttonClicked, [this](bool checked) {
        if (checked) {
          FrogPilotConfirmationDialog::toggleAlert(
          tr("WARNING: This is very experimental and may cause the car to not brake or stop safely! Please report any issues in the FrogPilot Discord!"),
          tr("I understand the risks."), this);
        }
      });

    } else if (param == "MTSCEnabled") {
      FrogPilotParamManageControl *mtscToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(mtscToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(mtscKeys.find(key.c_str()) != mtscKeys.end());
        }
      });
      toggle = mtscToggle;
    } else if (param == "MTSCAggressiveness") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 200, std::map<int, QString>(), this, false, "%");

    } else if (param == "ModelSelector") {
      FrogPilotParamManageControl *modelsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(modelsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(false);
        }

        deleteModelBtn->setVisible(true);
        downloadModelBtn->setVisible(true);
        selectModelBtn->setVisible(true);
      });
      toggle = modelsToggle;

      QDir modelDir("/data/models/");

      deleteModelBtn = new ButtonControl(tr("Delete Model"), tr("DELETE"), "");
      QObject::connect(deleteModelBtn, &ButtonControl::clicked, [=]() {
        std::string currentModel = params.get("Model") + ".thneed";

        QStringList availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
        QStringList modelLabels = QString::fromStdString(params.get("AvailableModelsNames")).split(",");

        QStringList existingModelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        QMap<QString, QString> labelToFileMap;
        QStringList deletableModelLabels;
        for (int i = 0; i < availableModels.size(); ++i) {
          QString modelFileName = availableModels[i] + ".thneed";
          if (existingModelFiles.contains(modelFileName) && modelFileName != QString::fromStdString(currentModel)) {
            QString readableName = modelLabels[i];
            deletableModelLabels.append(readableName);
            labelToFileMap[readableName] = modelFileName;
          }
        }

        QString selectedModel = MultiOptionDialog::getSelection(tr("Select a model to delete"), deletableModelLabels, "", this);
        if (!selectedModel.isEmpty() && ConfirmationDialog::confirm(tr("Are you sure you want to delete this model?"), tr("Delete"), this)) {
          std::thread([=]() {
            deleteModelBtn->setValue(tr("Deleting..."));

            deleteModelBtn->setEnabled(false);
            downloadModelBtn->setEnabled(false);
            selectModelBtn->setEnabled(false);

            QString modelToDelete = labelToFileMap[selectedModel];

            QFile::remove(modelDir.absoluteFilePath(modelToDelete));

            deleteModelBtn->setEnabled(true);
            downloadModelBtn->setEnabled(true);
            selectModelBtn->setEnabled(true);

            deleteModelBtn->setValue(tr("Deleted!"));
            std::this_thread::sleep_for(std::chrono::seconds(3));
            deleteModelBtn->setValue("");
          }).detach();
        }
      });
      addItem(deleteModelBtn);

      downloadModelBtn = new ButtonControl(tr("Download Model"), tr("DOWNLOAD"), "");
      QObject::connect(downloadModelBtn, &ButtonControl::clicked, [=]() {
        QStringList availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
        QStringList modelLabels = QString::fromStdString(params.get("AvailableModelsNames")).split(",");

        QMap<QString, QString> labelToModelMap;
        QStringList downloadableModelLabels;
        QStringList existingModelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        for (int i = 0; i < availableModels.size(); ++i) {
          QString modelFileName = availableModels.at(i) + ".thneed";
          if (!existingModelFiles.contains(modelFileName)) {
            QString readableName = modelLabels.at(i);
            if (!readableName.contains("(Default)")) {
              downloadableModelLabels.append(readableName);
              labelToModelMap.insert(readableName, availableModels.at(i));
            }
          }
        }

        QString modelToDownload = MultiOptionDialog::getSelection(tr("Select a driving model to download"), downloadableModelLabels, "", this);
        if (!modelToDownload.isEmpty()) {
          QString selectedModelValue = labelToModelMap.value(modelToDownload);
          paramsMemory.put("ModelToDownload", selectedModelValue.toStdString());

          deleteModelBtn->setEnabled(false);
          downloadModelBtn->setEnabled(false);
          selectModelBtn->setEnabled(false);

          QTimer *failureTimer = new QTimer(this);
          failureTimer->setSingleShot(true);

          QTimer *progressTimer = new QTimer(this);
          progressTimer->setInterval(100);

          connect(failureTimer, &QTimer::timeout, this, [=]() {
            deleteModelBtn->setEnabled(true);
            downloadModelBtn->setEnabled(true);
            selectModelBtn->setEnabled(true);

            downloadModelBtn->setValue(tr("Download failed..."));
            paramsMemory.remove("ModelDownloadProgress");
            paramsMemory.remove("ModelToDownload");

            progressTimer->stop();
            progressTimer->deleteLater();

            QTimer::singleShot(3000, this, [this]() {
              downloadModelBtn->setValue("");
            });
          });

          connect(progressTimer, &QTimer::timeout, this, [=]() mutable {
            static int lastProgress = -1;
            int progress = paramsMemory.getInt("ModelDownloadProgress");

            if (progress == lastProgress) {
              if (!failureTimer->isActive()) {
                failureTimer->start(30000);
              }
            } else {
              lastProgress = progress;
              downloadModelBtn->setValue(QString::number(progress) + "%");
              failureTimer->stop();

              if (progress == 100) {
                deleteModelBtn->setEnabled(true);
                downloadModelBtn->setEnabled(true);
                selectModelBtn->setEnabled(true);

                downloadModelBtn->setValue(tr("Downloaded!"));
                paramsMemory.remove("ModelDownloadProgress");
                paramsMemory.remove("ModelToDownload");

                progressTimer->stop();
                progressTimer->deleteLater();

                QTimer::singleShot(3000, this, [this]() {
                  if (paramsMemory.get("ModelDownloadProgress").empty()) {
                    downloadModelBtn->setValue("");
                  }
                });
              }
            }
          });
          progressTimer->start();
        }
      });
      addItem(downloadModelBtn);

      selectModelBtn = new ButtonControl(tr("Select Model"), tr("SELECT"), "");
      QObject::connect(selectModelBtn, &ButtonControl::clicked, [=]() {
        QStringList availableModels = QString::fromStdString(params.get("AvailableModels")).split(",");
        QStringList modelLabels = QString::fromStdString(params.get("AvailableModelsNames")).split(",");

        QStringList modelFiles = modelDir.entryList({"*.thneed"}, QDir::Files);
        QSet<QString> modelFilesBaseNames;
        for (const QString &modelFile : modelFiles) {
          modelFilesBaseNames.insert(modelFile.section('.', 0, 0));
        }

        QStringList selectableModelLabels;
        for (int i = 0; i < availableModels.size(); ++i) {
          if (modelFilesBaseNames.contains(availableModels[i]) || modelLabels[i].contains("(Default)")) {
            selectableModelLabels.append(modelLabels[i]);
          }
        }

        QString modelToSelect = MultiOptionDialog::getSelection(tr("Select a model - 🗺️ = Navigation | 📡 = Radar | 👀 = VOACC"), selectableModelLabels, "", this);
        if (!modelToSelect.isEmpty()) {
          selectModelBtn->setValue(modelToSelect);

          int modelIndex = modelLabels.indexOf(modelToSelect);
          if (modelIndex != -1) {
            QString selectedModel = availableModels.at(modelIndex);
            params.putNonBlocking("Model", selectedModel.toStdString());
            params.putNonBlocking("ModelName", modelToSelect.toStdString());
          }

          if (FrogPilotConfirmationDialog::yesorno(tr("Do you want to start with a fresh calibration for the newly selected model?"), this)) {
            params.remove("CalibrationParams");
            params.remove("LiveTorqueParameters");
          }

          if (started) {
            if (FrogPilotConfirmationDialog::toggle(tr("Reboot required to take effect."), tr("Reboot Now"), this)) {
              Hardware::reboot();
            }
          }
        }
      });
      addItem(selectModelBtn);
      selectModelBtn->setValue(QString::fromStdString(params.get("ModelName")));

    } else if (param == "QOLControls") {
      FrogPilotParamManageControl *qolToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(qolToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedQolKeys = qolKeys;

          if (!hasPCMCruise) {
            modifiedQolKeys.erase("ReverseCruise");
          } else {
            modifiedQolKeys.erase("CustomCruise");
            modifiedQolKeys.erase("CustomCruiseLong");
            modifiedQolKeys.erase("SetSpeedOffset");
          }

          if (!isToyota && !isGM && !isHKGCanFd) {
            modifiedQolKeys.erase("MapGears");
          }

          toggle->setVisible(modifiedQolKeys.find(key.c_str()) != modifiedQolKeys.end());
        }
      });
      toggle = qolToggle;
    } else if (param == "CustomCruise") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, tr(" mph"));
    } else if (param == "CustomCruiseLong") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 99, std::map<int, QString>(), this, false, tr(" mph"));
    } else if (param == "MapGears") {
      std::vector<QString> mapGearsToggles{"MapAcceleration", "MapDeceleration"};
      std::vector<QString> mapGearsToggleNames{tr("Acceleration"), tr("Deceleration")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, mapGearsToggles, mapGearsToggleNames);
    } else if (param == "PauseLateralSpeed") {
      std::vector<QString> pauseLateralToggles{"PauseLateralOnSignal"};
      std::vector<QString> pauseLateralToggleNames{"Turn Signal Only"};
      toggle = new FrogPilotParamValueToggleControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr(" mph"), 1, 1, pauseLateralToggles, pauseLateralToggleNames);
    } else if (param == "PauseLateralOnSignal") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr(" mph"));
    } else if (param == "ReverseCruise") {
      std::vector<QString> reverseCruiseToggles{"ReverseCruiseUI"};
      std::vector<QString> reverseCruiseNames{tr("Control Via UI")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, reverseCruiseToggles, reverseCruiseNames);
    } else if (param == "SetSpeedOffset") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr(" mph"));

    } else if (param == "LaneChangeCustomizations") {
      FrogPilotParamManageControl *laneChangeToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(laneChangeToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(laneChangeKeys.find(key.c_str()) != laneChangeKeys.end());
        }
      });
      toggle = laneChangeToggle;
    } else if (param == "MinimumLaneChangeSpeed") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 99, std::map<int, QString>(), this, false, tr(" mph"));
    } else if (param == "LaneChangeTime") {
      std::map<int, QString> laneChangeTimeLabels;
      for (int i = 0; i <= 10; ++i) {
        laneChangeTimeLabels[i] = i == 0 ? "Instant" : QString::number(i / 2.0) + " seconds";
      }
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 10, laneChangeTimeLabels, this, false);
    } else if (param == "LaneDetectionWidth") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 100, std::map<int, QString>(), this, false, " feet", 10);

    } else if (param == "SpeedLimitController") {
      FrogPilotParamManageControl *speedLimitControllerToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(speedLimitControllerToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        slcOpen = true;
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end());
        }
      });
      toggle = speedLimitControllerToggle;
    } else if (param == "SLCControls") {
      FrogPilotParamManageControl *manageSLCControlsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCControlsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerControlsKeys.find(key.c_str()) != speedLimitControllerControlsKeys.end());
          openSubParentToggle();
        }
      });
      toggle = manageSLCControlsToggle;
    } else if (param == "SLCQOL") {
      FrogPilotParamManageControl *manageSLCQOLToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCQOLToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          std::set<QString> modifiedSpeedLimitControllerQOLKeys = speedLimitControllerQOLKeys;

          if (hasPCMCruise) {
            modifiedSpeedLimitControllerQOLKeys.erase("SetSpeedLimit");
          }

          if (!isToyota) {
            modifiedSpeedLimitControllerQOLKeys.erase("ForceMPHDashboard");
          }

          toggle->setVisible(modifiedSpeedLimitControllerQOLKeys.find(key.c_str()) != modifiedSpeedLimitControllerQOLKeys.end());
          openSubParentToggle();
        }
      });
      toggle = manageSLCQOLToggle;
    } else if (param == "SLCConfirmation") {
      std::vector<QString> slcConfirmationToggles{"SLCConfirmationLower", "SLCConfirmationHigher"};
      std::vector<QString> slcConfirmationNames{tr("Lower Limits"), tr("Higher Limits")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, slcConfirmationToggles, slcConfirmationNames);
    } else if (param == "SLCLookaheadHigher" || param == "SLCLookaheadLower") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 0, 60, std::map<int, QString>(), this, false, " seconds");
    } else if (param == "SLCVisuals") {
      FrogPilotParamManageControl *manageSLCVisualsToggle = new FrogPilotParamManageControl(param, title, desc, icon, this, true);
      QObject::connect(manageSLCVisualsToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(speedLimitControllerVisualsKeys.find(key.c_str()) != speedLimitControllerVisualsKeys.end());
          openSubParentToggle();
        }
      });
      toggle = manageSLCVisualsToggle;
    } else if (param == "Offset1" || param == "Offset2" || param == "Offset3" || param == "Offset4") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, -99, 99, std::map<int, QString>(), this, false, tr(" mph"));
    } else if (param == "ShowSLCOffset") {
      std::vector<QString> slcOffsetToggles{"ShowSLCOffsetUI"};
      std::vector<QString> slcOffsetToggleNames{tr("Control Via UI")};
      toggle = new FrogPilotParamToggleControl(param, title, desc, icon, slcOffsetToggles, slcOffsetToggleNames);
    } else if (param == "SLCFallback") {
      std::vector<QString> fallbackOptions{tr("Set Speed"), tr("Experimental Mode"), tr("Previous Limit")};
      FrogPilotButtonParamControl *fallbackSelection = new FrogPilotButtonParamControl(param, title, desc, icon, fallbackOptions);
      toggle = fallbackSelection;
    } else if (param == "SLCOverride") {
      std::vector<QString> overrideOptions{tr("None"), tr("Manual Set Speed"), tr("Set Speed")};
      FrogPilotButtonParamControl *overrideSelection = new FrogPilotButtonParamControl(param, title, desc, icon, overrideOptions);
      toggle = overrideSelection;
    } else if (param == "SLCPriority") {
      ButtonControl *slcPriorityButton = new ButtonControl(title, tr("SELECT"), desc);
      QStringList primaryPriorities = {tr("None"), tr("Dashboard"), tr("Navigation"), tr("Offline Maps"), tr("Highest"), tr("Lowest")};
      QStringList secondaryTertiaryPriorities = {tr("None"), tr("Dashboard"), tr("Navigation"), tr("Offline Maps")};
      QStringList priorityPrompts = {tr("Select your primary priority"), tr("Select your secondary priority"), tr("Select your tertiary priority")};

      QObject::connect(slcPriorityButton, &ButtonControl::clicked, [=]() {
        QStringList selectedPriorities;

        for (int i = 1; i <= 3; ++i) {
          QStringList currentPriorities = (i == 1) ? primaryPriorities : secondaryTertiaryPriorities;
          QStringList prioritiesToDisplay = currentPriorities;
          for (const auto &selectedPriority : qAsConst(selectedPriorities)) {
            prioritiesToDisplay.removeAll(selectedPriority);
          }

          if (!hasDashSpeedLimits) {
            prioritiesToDisplay.removeAll(tr("Dashboard"));
          }

          if (prioritiesToDisplay.size() == 1 && prioritiesToDisplay.contains(tr("None"))) {
            break;
          }

          QString priorityKey = QString("SLCPriority%1").arg(i);
          QString selection = MultiOptionDialog::getSelection(priorityPrompts[i - 1], prioritiesToDisplay, "", this);

          if (selection.isEmpty()) break;

          params.putNonBlocking(priorityKey.toStdString(), selection.toStdString());
          selectedPriorities.append(selection);

          if (selection == tr("Lowest") || selection == tr("Highest") || selection == tr("None")) break;

          updateFrogPilotToggles();
        }

        selectedPriorities.removeAll(tr("None"));
        slcPriorityButton->setValue(selectedPriorities.join(", "));
      });

      QStringList initialPriorities;
      for (int i = 1; i <= 3; ++i) {
        QString priorityKey = QString("SLCPriority%1").arg(i);
        QString priority = QString::fromStdString(params.get(priorityKey.toStdString()));

        if (!priority.isEmpty() && primaryPriorities.contains(priority) && priority != tr("None")) {
          initialPriorities.append(priority);
        }
      }
      slcPriorityButton->setValue(initialPriorities.join(", "));
      toggle = slcPriorityButton;

    } else if (param == "VisionTurnControl") {
      FrogPilotParamManageControl *visionTurnControlToggle = new FrogPilotParamManageControl(param, title, desc, icon, this);
      QObject::connect(visionTurnControlToggle, &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
        openParentToggle();
        for (auto &[key, toggle] : toggles) {
          toggle->setVisible(visionTurnControlKeys.find(key.c_str()) != visionTurnControlKeys.end());
        }
      });
      toggle = visionTurnControlToggle;
    } else if (param == "CurveSensitivity" || param == "TurnAggressiveness") {
      toggle = new FrogPilotParamValueControl(param, title, desc, icon, 1, 200, std::map<int, QString>(), this, false, "%");

    } else {
      toggle = new ParamControl(param, title, desc, icon, this);
    }

    addItem(toggle);
    toggles[param.toStdString()] = toggle;

    QObject::connect(static_cast<ToggleControl*>(toggle), &ToggleControl::toggleFlipped, &updateFrogPilotToggles);
    QObject::connect(static_cast<FrogPilotParamValueControl*>(toggle), &FrogPilotParamValueControl::valueChanged, &updateFrogPilotToggles);

    ParamWatcher *param_watcher = new ParamWatcher(this);
    param_watcher->addParam("CESpeed");
    param_watcher->addParam("CESpeedLead");

    QObject::connect(param_watcher, &ParamWatcher::paramChanged, [=](const QString &param_name, const QString &param_value) {
      updateFrogPilotToggles();
    });

    QObject::connect(toggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });

    QObject::connect(static_cast<FrogPilotParamManageControl*>(toggle), &FrogPilotParamManageControl::manageButtonClicked, this, [this]() {
      update();
    });
  }

  QObject::connect(static_cast<ToggleControl*>(toggles["IncreaseThermalLimits"]), &ToggleControl::toggleFlipped, [this]() {
    if (params.getBool("IncreaseThermalLimits")) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This can cause premature wear or damage by running the device over comma's recommended temperature limits!"),
        tr("I understand the risks."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoLogging"]), &ToggleControl::toggleFlipped, [this]() {
    if (params.getBool("NoLogging")) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This will prevent your drives from being recorded and the data will be unobtainable!"),
        tr("I understand the risks."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["NoUploads"]), &ToggleControl::toggleFlipped, [this]() {
    if (params.getBool("NoUploads")) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("WARNING: This will prevent your drives from appearing on comma connect which may impact debugging and support!"),
        tr("I understand the risks."), this);
    }
  });

  QObject::connect(static_cast<ToggleControl*>(toggles["TrafficMode"]), &ToggleControl::toggleFlipped, [this]() {
    if (params.getBool("TrafficMode")) {
      FrogPilotConfirmationDialog::toggleAlert(
        tr("To activate 'Traffic Mode' you hold down the 'distance' button on your steering wheel for 2.5 seconds."),
        tr("Sounds good!"), this);
    }
  });

  std::set<QString> rebootKeys = {"AlwaysOnLateral", "NNFF", "NNFFLite"};
  for (const QString &key : rebootKeys) {
    QObject::connect(static_cast<ToggleControl*>(toggles[key.toStdString().c_str()]), &ToggleControl::toggleFlipped, [this]() {
      if (started) {
        if (FrogPilotConfirmationDialog::toggle(tr("Reboot required to take effect."), tr("Reboot Now"), this)) {
          Hardware::reboot();
        }
      }
    });
  }

  FrogPilotParamValueControl *trafficFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficFollow"]);
  FrogPilotParamValueControl *trafficAccelerationoggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkAcceleration"]);
  FrogPilotParamValueControl *trafficSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["TrafficJerkSpeed"]);
  FrogPilotButtonsControl *trafficResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetTrafficPersonality"]);

  QObject::connect(trafficResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("Are you sure you want to completely reset your settings for the 'Traffic Mode' personality?"), this)) {
      params.putFloat("TrafficFollow", 0.5);
      params.putFloat("TrafficJerkAcceleration", 50);
      params.putFloat("TrafficJerkSpeed", 75);
      trafficFollowToggle->refresh();
      trafficAccelerationoggle->refresh();
      trafficSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *aggressiveFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveFollow"]);
  FrogPilotParamValueControl *aggressiveAccelerationoggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkAcceleration"]);
  FrogPilotParamValueControl *aggressiveSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["AggressiveJerkSpeed"]);
  FrogPilotButtonsControl *aggressiveResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetAggressivePersonality"]);

  QObject::connect(aggressiveResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("Are you sure you want to completely reset your settings for the 'Aggressive' personality?"), this)) {
      params.putFloat("AggressiveFollow", 1.25);
      params.putFloat("AggressiveJerkAcceleration", 50);
      params.putFloat("AggressiveJerkSpeed", 50);
      aggressiveFollowToggle->refresh();
      aggressiveAccelerationoggle->refresh();
      aggressiveSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *standardFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardFollow"]);
  FrogPilotParamValueControl *standardAccelerationoggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkAcceleration"]);
  FrogPilotParamValueControl *standardSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["StandardJerkSpeed"]);
  FrogPilotButtonsControl *standardResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetStandardPersonality"]);

  QObject::connect(standardResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("Are you sure you want to completely reset your settings for the 'Standard' personality?"), this)) {
      params.putFloat("StandardFollow", 1.45);
      params.putFloat("StandardJerkAcceleration", 100);
      params.putFloat("StandardJerkSpeed", 100);
      standardFollowToggle->refresh();
      standardAccelerationoggle->refresh();
      standardSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  FrogPilotParamValueControl *relaxedFollowToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedFollow"]);
  FrogPilotParamValueControl *relaxedAccelerationoggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkAcceleration"]);
  FrogPilotParamValueControl *relaxedSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["RelaxedJerkSpeed"]);
  FrogPilotButtonsControl *relaxedResetButton = static_cast<FrogPilotButtonsControl*>(toggles["ResetRelaxedPersonality"]);

  QObject::connect(relaxedResetButton, &FrogPilotButtonsControl::buttonClicked, this, [=]() {
    if (FrogPilotConfirmationDialog::yesorno(tr("Are you sure you want to completely reset your settings for the 'Relaxed' personality?"), this)) {
      params.putFloat("RelaxedFollow", 1.75);
      params.putFloat("RelaxedJerkAcceleration", 100);
      params.putFloat("RelaxedJerkSpeed", 100);
      relaxedFollowToggle->refresh();
      relaxedAccelerationoggle->refresh();
      relaxedSpeedToggle->refresh();
      updateFrogPilotToggles();
    }
  });

  modelManagerToggle = static_cast<FrogPilotParamManageControl*>(toggles["ModelSelector"]);
  steerRatioToggle = static_cast<FrogPilotParamValueToggleControl*>(toggles["SteerRatio"]);

  QObject::connect(steerRatioToggle, &FrogPilotParamValueToggleControl::buttonClicked, this, [this]() {
    params.putFloat("SteerRatio", steerRatioStock);
    params.putBool("ResetSteerRatio", false);
    steerRatioToggle->refresh();
    updateFrogPilotToggles();
  });

  QObject::connect(parent, &SettingsWindow::closeParentToggle, this, &FrogPilotControlsPanel::hideToggles);
  QObject::connect(parent, &SettingsWindow::closeSubParentToggle, this, &FrogPilotControlsPanel::hideSubToggles);
  QObject::connect(parent, &SettingsWindow::closeSubSubParentToggle, this, &FrogPilotControlsPanel::hideSubSubToggles);
  QObject::connect(parent, &SettingsWindow::updateMetric, this, &FrogPilotControlsPanel::updateMetric);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &FrogPilotControlsPanel::updateCarToggles);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotControlsPanel::updateState);

  updateMetric();
}

void FrogPilotControlsPanel::showEvent(QShowEvent *event, const UIState &s) {
  hasOpenpilotLongitudinal = hasOpenpilotLongitudinal && !params.getBool("DisableOpenpilotLongitudinal");

  downloadModelBtn->setEnabled(s.scene.online);
}

void FrogPilotControlsPanel::updateState(const UIState &s) {
  if (!isVisible()) return;

  started = s.scene.started;

  modelManagerToggle->setEnabled(!s.scene.started || s.scene.parked);
}

void FrogPilotControlsPanel::updateCarToggles() {
  auto carParams = params.get("CarParamsPersistent");
  if (!carParams.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(carParams.data(), carParams.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();
    auto carFingerprint = CP.getCarFingerprint();
    auto carName = CP.getCarName();
    auto safetyConfigs = CP.getSafetyConfigs();
    auto safetyModel = safetyConfigs[0].getSafetyModel();

    hasAutoTune = (carName == "hyundai" || carName == "toyota") && CP.getLateralTuning().which() == cereal::CarParams::LateralTuning::TORQUE;
    uiState()->scene.has_auto_tune = hasAutoTune;
    hasCommaNNFFSupport = checkCommaNNFFSupport(carFingerprint);
    hasDashSpeedLimits = carName == "hyundai" || carName == "toyota";
    hasNNFFLog = checkNNFFLogFileExists(carFingerprint);
    hasOpenpilotLongitudinal = (CP.getOpenpilotLongitudinalControl() && !params.getBool("DisableOpenpilotLongitudinal")) || params.getBool("CSLCEnabled");
    hasPCMCruise = CP.getPcmCruise() && !params.getBool("CSLCEnabled");
    isGM = carName == "gm";
    isHKGCanFd = (carName == "hyundai") && (safetyModel == cereal::CarParams::SafetyModel::HYUNDAI_CANFD);
    isToyota = carName == "toyota";
    steerRatioStock = CP.getSteerRatio();

    steerRatioToggle->setTitle(QString(tr("Steer Ratio (Default: %1)")).arg(QString::number(steerRatioStock, 'f', 2)));
    steerRatioToggle->updateControl(steerRatioStock * 0.75, steerRatioStock * 1.25, "", 0.01);
    steerRatioToggle->refresh();
  } else {
    hasAutoTune = false;
    hasCommaNNFFSupport = false;
    hasDashSpeedLimits = true;
    hasNNFFLog = true;
    hasOpenpilotLongitudinal = true;
    hasPCMCruise = true;
    isGM = true;
    isHKGCanFd = true;
    isToyota = true;
  }

  hideToggles();
}

void FrogPilotControlsPanel::updateMetric() {
  bool previousIsMetric = isMetric;
  isMetric = params.getBool("IsMetric");

  if (isMetric != previousIsMetric) {
    double distanceConversion = isMetric ? FOOT_TO_METER : METER_TO_FOOT;
    double speedConversion = isMetric ? MILE_TO_KM : KM_TO_MILE;

    params.putIntNonBlocking("CESpeed", std::nearbyint(params.getInt("CESpeed") * speedConversion));
    params.putIntNonBlocking("CESpeedLead", std::nearbyint(params.getInt("CESpeedLead") * speedConversion));
    params.putIntNonBlocking("CustomCruise", std::nearbyint(params.getInt("CustomCruise") * speedConversion));
    params.putIntNonBlocking("CustomCruiseLong", std::nearbyint(params.getInt("CustomCruiseLong") * speedConversion));
    params.putIntNonBlocking("LaneDetectionWidth", std::nearbyint(params.getInt("LaneDetectionWidth") * distanceConversion));
    params.putIntNonBlocking("MinimumLaneChangeSpeed", std::nearbyint(params.getInt("MinimumLaneChangeSpeed") * speedConversion));
    params.putIntNonBlocking("Offset1", std::nearbyint(params.getInt("Offset1") * speedConversion));
    params.putIntNonBlocking("Offset2", std::nearbyint(params.getInt("Offset2") * speedConversion));
    params.putIntNonBlocking("Offset3", std::nearbyint(params.getInt("Offset3") * speedConversion));
    params.putIntNonBlocking("Offset4", std::nearbyint(params.getInt("Offset4") * speedConversion));
    params.putIntNonBlocking("PauseAOLOnBrake", std::nearbyint(params.getInt("PauseAOLOnBrake") * speedConversion));
    params.putIntNonBlocking("PauseLateralOnSignal", std::nearbyint(params.getInt("PauseLateralOnSignal") * speedConversion));
    params.putIntNonBlocking("PauseLateralSpeed", std::nearbyint(params.getInt("PauseLateralSpeed") * speedConversion));
    params.putIntNonBlocking("SetSpeedOffset", std::nearbyint(params.getInt("SetSpeedOffset") * speedConversion));
    params.putIntNonBlocking("StoppingDistance", std::nearbyint(params.getInt("StoppingDistance") * distanceConversion));
  }

  FrogPilotParamValueControl *customCruiseToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruise"]);
  FrogPilotParamValueControl *customCruiseLongToggle = static_cast<FrogPilotParamValueControl*>(toggles["CustomCruiseLong"]);
  FrogPilotParamValueControl *laneWidthToggle = static_cast<FrogPilotParamValueControl*>(toggles["LaneDetectionWidth"]);
  FrogPilotParamValueControl *minimumLaneChangeSpeedToggle = static_cast<FrogPilotParamValueControl*>(toggles["MinimumLaneChangeSpeed"]);
  FrogPilotParamValueControl *offset1Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset1"]);
  FrogPilotParamValueControl *offset2Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset2"]);
  FrogPilotParamValueControl *offset3Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset3"]);
  FrogPilotParamValueControl *offset4Toggle = static_cast<FrogPilotParamValueControl*>(toggles["Offset4"]);
  FrogPilotParamValueControl *pauseAOLOnBrakeToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseAOLOnBrake"]);
  FrogPilotParamValueControl *pauseLateralToggle = static_cast<FrogPilotParamValueControl*>(toggles["PauseLateralSpeed"]);
  FrogPilotParamValueControl *setSpeedOffsetToggle = static_cast<FrogPilotParamValueControl*>(toggles["SetSpeedOffset"]);
  FrogPilotParamValueControl *stoppingDistanceToggle = static_cast<FrogPilotParamValueControl*>(toggles["StoppingDistance"]);

  if (isMetric) {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 kph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 kph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 kph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 kph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 kph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 kph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 kph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 kph."));

    customCruiseToggle->updateControl(1, 150, tr(" kph"));
    customCruiseLongToggle->updateControl(1, 150, tr(" kph"));
    minimumLaneChangeSpeedToggle->updateControl(0, 150, tr(" kph"));
    offset1Toggle->updateControl(-99, 99, tr(" kph"));
    offset2Toggle->updateControl(-99, 99, tr(" kph"));
    offset3Toggle->updateControl(-99, 99, tr(" kph"));
    offset4Toggle->updateControl(-99, 99, tr(" kph"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr(" kph"));
    pauseLateralToggle->updateControl(0, 99, tr(" kph"));
    setSpeedOffsetToggle->updateControl(0, 150, tr(" kph"));

    laneWidthToggle->updateControl(0, 30, tr(" meters"), 10);
    stoppingDistanceToggle->updateControl(0, 5, tr(" meters"));
  } else {
    offset1Toggle->setTitle(tr("Speed Limit Offset (0-34 mph)"));
    offset2Toggle->setTitle(tr("Speed Limit Offset (35-54 mph)"));
    offset3Toggle->setTitle(tr("Speed Limit Offset (55-64 mph)"));
    offset4Toggle->setTitle(tr("Speed Limit Offset (65-99 mph)"));

    offset1Toggle->setDescription(tr("Set speed limit offset for limits between 0-34 mph."));
    offset2Toggle->setDescription(tr("Set speed limit offset for limits between 35-54 mph."));
    offset3Toggle->setDescription(tr("Set speed limit offset for limits between 55-64 mph."));
    offset4Toggle->setDescription(tr("Set speed limit offset for limits between 65-99 mph."));

    customCruiseToggle->updateControl(1, 99, tr(" mph"));
    customCruiseLongToggle->updateControl(1, 99, tr(" mph"));
    minimumLaneChangeSpeedToggle->updateControl(0, 99, tr(" mph"));
    offset1Toggle->updateControl(-99, 99, tr(" mph"));
    offset2Toggle->updateControl(-99, 99, tr(" mph"));
    offset3Toggle->updateControl(-99, 99, tr(" mph"));
    offset4Toggle->updateControl(-99, 99, tr(" mph"));
    pauseAOLOnBrakeToggle->updateControl(0, 99, tr(" mph"));
    pauseLateralToggle->updateControl(0, 99, tr(" mph"));
    setSpeedOffsetToggle->updateControl(0, 99, tr(" mph"));

    laneWidthToggle->updateControl(0, 100, tr(" feet"), 10);
    stoppingDistanceToggle->updateControl(0, 10, tr(" feet"));
  }

  customCruiseToggle->refresh();
  customCruiseLongToggle->refresh();
  laneWidthToggle->refresh();
  minimumLaneChangeSpeedToggle->refresh();
  offset1Toggle->refresh();
  offset2Toggle->refresh();
  offset3Toggle->refresh();
  offset4Toggle->refresh();
  pauseAOLOnBrakeToggle->refresh();
  pauseLateralToggle->refresh();
  setSpeedOffsetToggle->refresh();
  stoppingDistanceToggle->refresh();
}

void FrogPilotControlsPanel::hideToggles() {
  customPersonalitiesOpen = false;
  slcOpen = false;

  conditionalSpeedsImperial->setVisible(false);
  conditionalSpeedsMetric->setVisible(false);
  deleteModelBtn->setVisible(false);
  downloadModelBtn->setVisible(false);
  personalitiesInfoBtn->setVisible(false);
  selectModelBtn->setVisible(false);

  std::set<QString> longitudinalKeys = {"ConditionalExperimental", "DrivingPersonalities", "ExperimentalModeActivation",
                                        "LongitudinalTune", "MTSCEnabled", "SpeedLimitController", "VisionTurnControl"};

  for (auto &[key, toggle] : toggles) {
    toggle->setVisible(false);

    if (!hasOpenpilotLongitudinal && longitudinalKeys.find(key.c_str()) != longitudinalKeys.end()) {
      continue;
    }

    bool subToggles = aggressivePersonalityKeys.find(key.c_str()) != aggressivePersonalityKeys.end() ||
                      aolKeys.find(key.c_str()) != aolKeys.end() ||
                      conditionalExperimentalKeys.find(key.c_str()) != conditionalExperimentalKeys.end() ||
                      customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end() ||
                      relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end() ||
                      deviceManagementKeys.find(key.c_str()) != deviceManagementKeys.end() ||
                      drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end() ||
                      experimentalModeActivationKeys.find(key.c_str()) != experimentalModeActivationKeys.end() ||
                      laneChangeKeys.find(key.c_str()) != laneChangeKeys.end() ||
                      lateralTuneKeys.find(key.c_str()) != lateralTuneKeys.end() ||
                      longitudinalTuneKeys.find(key.c_str()) != longitudinalTuneKeys.end() ||
                      mtscKeys.find(key.c_str()) != mtscKeys.end() ||
                      qolKeys.find(key.c_str()) != qolKeys.end() ||
                      relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end() ||
                      speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end() ||
                      speedLimitControllerControlsKeys.find(key.c_str()) != speedLimitControllerControlsKeys.end() ||
                      speedLimitControllerQOLKeys.find(key.c_str()) != speedLimitControllerQOLKeys.end() ||
                      speedLimitControllerVisualsKeys.find(key.c_str()) != speedLimitControllerVisualsKeys.end() ||
                      standardPersonalityKeys.find(key.c_str()) != standardPersonalityKeys.end() ||
                      relaxedPersonalityKeys.find(key.c_str()) != relaxedPersonalityKeys.end() ||
                      trafficPersonalityKeys.find(key.c_str()) != trafficPersonalityKeys.end() ||
                      tuningKeys.find(key.c_str()) != tuningKeys.end() ||
                      visionTurnControlKeys.find(key.c_str()) != visionTurnControlKeys.end();
    toggle->setVisible(!subToggles);
  }

  update();
}

void FrogPilotControlsPanel::hideSubToggles() {
  if (customPersonalitiesOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = drivingPersonalityKeys.find(key.c_str()) != drivingPersonalityKeys.end();
      toggle->setVisible(isVisible);
    }
  } else if (slcOpen) {
    for (auto &[key, toggle] : toggles) {
      bool isVisible = speedLimitControllerKeys.find(key.c_str()) != speedLimitControllerKeys.end();
      toggle->setVisible(isVisible);
    }
  }

  update();
}

void FrogPilotControlsPanel::hideSubSubToggles() {
  personalitiesInfoBtn->setVisible(false);

  for (auto &[key, toggle] : toggles) {
    bool isVisible = customdrivingPersonalityKeys.find(key.c_str()) != customdrivingPersonalityKeys.end();
    toggle->setVisible(isVisible);
  }

  update();
}
