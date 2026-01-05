#!/usr/bin/env bash

export PASSIVE="0"
export NOBOARD="1"
export SIMULATION="1"
export SKIP_FW_QUERY="1"
export FINGERPRINT="BYD_HAN_EV_20"
export BYD_RADAR="1"

# 添加PYTHONPATH环境变量
SCRIPT_DIR=$(dirname "$0")
OPENPILOT_DIR=$SCRIPT_DIR/../../
export PYTHONPATH=$OPENPILOT_DIR:$PYTHONPATH

# 添加模拟器特定的环境变量
export DONGLE_ID="UnregisteredDevice"
export NO_CAMERA="1"

# 确保ControlsReady参数被正确设置
python3 -c "from openpilot.common.params import Params; Params().put_bool('ControlsReady', True)"

# 设置CarParams相关参数
python3 -c "from openpilot.common.params import Params; Params().put('CarParams', '')"

export BLOCK="${BLOCK},camerad,loggerd,encoderd,micd,logmessaged"
if [[ "$CI" ]]; then
  # TODO: offscreen UI should work
  export BLOCK="${BLOCK},ui"
fi

python3 -c "from openpilot.selfdrive.test.helpers import set_params_enabled; set_params_enabled()"

# 设置ControlsReady参数，确保模拟器环境中的CAN解析器能正常工作
python3 -c "from openpilot.common.params import Params; Params().put_bool('ControlsReady', True)"

# 设置DongleId参数
python3 -c "from openpilot.common.params import Params; Params().put('DongleId', 'UnregisteredDevice')"

# 添加调试信息，检查ControlsReady参数是否正确设置
python3 -c "from openpilot.common.params import Params; print('DEBUG: ControlsReady parameter set to:', Params().get_bool('ControlsReady'))"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
cd $OPENPILOT_DIR/system/manager && exec ./manager.py