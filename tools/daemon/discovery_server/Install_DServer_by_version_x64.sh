#!/bin/sh
set -e

# 帮助信息
if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "用法: sudo $0 <版本号> [daily]"
    echo "用法: sudo $0 stop"
    echo "示例: sudo $0 2.2.882-20250617.140"
    echo "示例: sudo $0 2.2.882-20250617.140 daily(测试版本)"
    echo "示例: sudo $0 stop"
    echo "参数说明:"
    echo "  <版本号>  必填，vbs包的版本号，如 2.2.882-20250617.140"
    echo "  [daily]   可选，测试版本指定使用 lvbs/test 仓库，默认为 lvbs/stable(稳定版本)"
    echo "  stop      停止 DServer 服务"
    exit 0
fi

# 检查依赖命令
for cmd in awk systemctl getent stat; do
    command -v $cmd >/dev/null 2>&1 || { echo "$cmd not found!"; exit 1; }
done

# 检查是否以 root 权限运行（系统服务配置需要 root 权限）
if [ "$(id -u)" != "0" ]; then
    echo "ERROR: This script requires root privileges"
    echo "Please run this script with sudo"
    exit 1
fi

# 处理 stop 命令
if [ "$1" = "stop" ]; then
    echo "Stopping DServer service..."
    
    # 检查服务是否存在
    if [ ! -f "/etc/systemd/system/dserver.service" ]; then
        echo "WARNING: DServer service file not found. Service may not be installed."
        exit 0
    fi
    
    # 检查服务是否正在运行
    if systemctl is-active --quiet dserver.service; then
        echo "DServer service is running. Stopping..."
        systemctl stop dserver.service
        if [ $? -eq 0 ]; then
            echo "DServer service stopped successfully."
        else
            echo "ERROR: Failed to stop DServer service"
            exit 1
        fi
    else
        echo "DServer service is not running."
    fi
    
    # 禁用服务（可选，让用户选择）
    echo "Do you want to disable DServer service from auto-start? (y/N)"
    read -r response
    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo "Disabling DServer service..."
        systemctl disable dserver.service
        if [ $? -eq 0 ]; then
            echo "DServer service disabled successfully."
        else
            echo "WARNING: Failed to disable DServer service"
        fi
    else
        echo "DServer service remains enabled for auto-start."
    fi
    
    # 显示服务状态
    echo "Current service status:"
    systemctl status dserver.service --no-pager
    
    exit 0
fi

echo "-----------------------1.Getting real user and home directory--------------------------"
# 获取真实用户和 home 目录
if [ ! -z "$SUDO_USER" ]; then
    REAL_USER="$SUDO_USER"
else
    REAL_USER="$USER"
fi
REAL_HOME=$(getent passwd "$REAL_USER" | cut -d: -f6)
echo "Debug: Real user: $REAL_USER"
echo "Debug: User home: $REAL_HOME"

# 检查 PATH 环境变量
echo "Debug: Current PATH: $PATH"

# 查找 conan 命令
CONAN_PATHS="/usr/local/bin/conan /usr/bin/conan $REAL_HOME/.local/bin/conan"
CONAN_PATH=""
for path in $CONAN_PATHS; do
    echo "Debug: Checking for conan at: $path"
    if [ -x "$path" ]; then
        CONAN_PATH="$path"
        echo "Debug: Found conan at: $CONAN_PATH"
        break
    fi
done
if [ -z "$CONAN_PATH" ]; then
    echo "ERROR: conan command not found in standard locations. Please install conan or add it to PATH"
    echo "Debug: Trying to find conan using find command..."
    CONAN_PATH=$(find /usr -name conan 2>/dev/null | head -1)
    if [ -z "$CONAN_PATH" ]; then
        exit 1
    else
        echo "Debug: Found conan using find at: $CONAN_PATH"
    fi
fi

echo "-----------------------2.Getting conan config home--------------------------"
# 获取 conan 的 home 目录（包数据目录）
CONAN_HOME=$(sudo -u "$REAL_USER" "$CONAN_PATH" config home 2>/dev/null)
if [ $? -ne 0 ] || [ -z "$CONAN_HOME" ]; then
    echo "ERROR: Failed to get conan config home directory"
    exit 1
fi
echo "Debug: Conan home: $CONAN_HOME"

# 检查 conan 配置
echo "Debug: Checking remote configuration..."
sudo -u "$REAL_USER" $CONAN_PATH remote list
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to list remote repositories"
    exit 1
fi

# 通过conan config home命令获取conan路径
echo "Debug: Getting conan config home..."
echo "Debug: Running command: $CONAN_PATH config home"
conan_dir=$(sudo -u "$REAL_USER" $CONAN_PATH config home)
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to get conan config home directory"
    exit 1
fi
echo "Debug: Conan directory: ${conan_dir}"


# 创建目录（如果不存在）
if [ ! -d "$conan_dir" ]; then
    echo "Warning: Creating conan directory: $conan_dir"
    mkdir -p "$conan_dir"
fi

# 解析参数
conan_version=$1
channel_param=$2

if [ -z "$conan_version" ]; then
    echo "ERROR: Version parameter is required"
    exit 1
fi

# 根据第二个参数决定使用哪个 channel
if [ "$channel_param" = "daily" ]; then
    CHANNEL="lvbs/test"
    echo "Debug: Using daily channel: $CHANNEL"
else
    CHANNEL="lvbs/stable"
    echo "Debug: Using stable channel: $CHANNEL"
fi

version=$(echo ${conan_version} | sed 's/vbs\///; s/@lvbs\/stable//; s/@lvbs\/test//')
echo "version=${version}"

# 假如已经安装过，可以忽略该步骤
echo "-----------------------3.Installing conan package--------------------------"
echo "Installing main package: ${conan_version}"

# 首先检查远程仓库中的包
echo "Debug: Checking available packages in remote repository..."
sudo -u "$REAL_USER" $CONAN_PATH search "vbs/*@$CHANNEL" -r liconan
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to search remote repository"
    exit 1
fi

# 检查特定版本是否存在
echo "Debug: Checking if version exists: vbs/${conan_version}@$CHANNEL"
sudo -u "$REAL_USER" $CONAN_PATH search "vbs/${conan_version}@$CHANNEL" -r liconan
if [ $? -ne 0 ]; then
    echo "ERROR: Package version ${conan_version} not found in remote repository"
    exit 1
fi

# 如果版本存在，继续安装
echo "Debug: Installing package..."
sudo -u "$REAL_USER" $CONAN_PATH install "vbs/${conan_version}@$CHANNEL" -s build_type=Release -pr=x64 -o security=True -o shared=True -r liconan

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to install main package ${conan_version}"
    exit 1
fi

echo "-----------------------4.Getting package info--------------------------"
# 执行conan info命令并捕获输出
echo "Getting package info for: ${conan_version}"
echo "Debug: Running command: $CONAN_PATH info vbs/${conan_version}@$CHANNEL -s build_type=Release -pr=x64 -o security=True -o shared=True -r liconan"
CONAN_OUTPUT=$(sudo -u "$REAL_USER" $CONAN_PATH info "vbs/${conan_version}@$CHANNEL" -s build_type=Release -pr=x64 -o security=True -o shared=True -r liconan)
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to get package info for ${conan_version}"
    exit 1
fi

# 打印完整的 conan info 输出以便调试
echo "Debug: Full conan info output:"
echo "$CONAN_OUTPUT"

# 提取ID值
echo "Debug: Extracting ID value..."
id_value_lib=$(echo "$CONAN_OUTPUT" | awk '
    /^vbs\// {vbs_section=1; next}
    /^[^ ]/ && vbs_section {vbs_section=0}
    vbs_section && /ID: / {
        gsub(/\[|\]/, "", $2)
        print $2
        exit
    }
')

echo "Debug: Raw ID value from awk: [${id_value_lib}]"
echo "Debug: CONAN_OUTPUT lines:"
echo "$CONAN_OUTPUT" | while IFS= read -r line; do
    echo "Line: $line"
done

# 检查是否找到了ID
if [ -z "$id_value_lib" ]; then
    echo "ERROR: Package version $conan_version not found or ID not found"
    echo "Debug: Try alternative method to get ID..."
    # 尝试另一种方式获取ID
    id_value_lib=$(echo "$CONAN_OUTPUT" | awk '
        /^vbs\// {vbs_section=1; next}
        /^[^ ]/ && vbs_section {vbs_section=0}
        vbs_section && /ID: / {
            gsub(/\[|\]/, "", $2)
            print $2
            exit
        }
    ')
    echo "Debug: Alternative ID value: [${id_value_lib}]"
    if [ -z "$id_value_lib" ]; then
        exit 1
    fi
else
    echo "ID for package $conan_version is: $id_value_lib"
fi


echo "-----------------------5.Installing DServer--------------------------"
if [ -d "${REAL_HOME}/dserver" ]; then
    echo "Removing existing directory: ${REAL_HOME}/dserver"
    rm -rf ${REAL_HOME}/dserver
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to remove existing directory ${REAL_HOME}/dserver"
        exit 1
    fi
fi

# 获取新版本的DServer
echo "Creating directory: ${REAL_HOME}/dserver"
mkdir -p ${REAL_HOME}/dserver
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create directory ${REAL_HOME}/dserver"
    exit 1
fi

echo "Copying files from: ${conan_dir}/data/vbs/${version}/$CHANNEL/package/$id_value_lib/*"
if [ ! -d "${conan_dir}/data/vbs/${version}/$CHANNEL/package/$id_value_lib" ]; then
    echo "ERROR: Source directory does not exist: ${conan_dir}/data/vbs/${version}/$CHANNEL/package/$id_value_lib"
    exit 1
fi

cp -r "${conan_dir}/data/vbs/${version}/$CHANNEL/package/$id_value_lib/"* "${REAL_HOME}/dserver"
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to copy files to ${REAL_HOME}/dserver"
    exit 1
fi

echo "Files copied successfully. Checking if DServer executable exists..."
if [ ! -f "${REAL_HOME}/dserver/tool/DServer" ]; then
    echo "ERROR: DServer executable not found in ${REAL_HOME}/dserver/tool"
    echo "DEBUG: Contents of ${REAL_HOME}/dserver/tool:"
    ls -la ${REAL_HOME}/dserver/tool
    exit 1
fi

# 启动DServer,并配置为开机自启动
echo "Configuring DServer service..."

# 检查是否以 root 权限运行（系统服务配置需要 root 权限）
if [ "$(id -u)" != "0" ]; then
    echo "ERROR: System service configuration requires root privileges"
    echo "Please run this script with sudo for the service configuration part"
    echo "You can run the conan operations separately as a regular user"
    exit 1
fi

# 检查dserver.service是否存在, 如果存在, 则停止并删除
if [ -f "/etc/systemd/system/dserver.service" ]; then
    echo "Stopping existing dserver.service..."
    systemctl stop dserver.service
    if [ $? -ne 0 ]; then
        echo "WARNING: Failed to stop existing dserver.service (may not be running)"
    fi
    
    echo "Disabling existing dserver.service..."
    systemctl disable dserver.service
    if [ $? -ne 0 ]; then
        echo "WARNING: Failed to disable existing dserver.service"
    fi
    
    echo "Removing existing dserver.service file..."
    rm -f /etc/systemd/system/dserver.service
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to remove existing dserver.service file"
        exit 1
    fi
fi

# 创建dserver.service文件，并写入开启自动DServer的配置
echo "Creating new dserver.service file..."
cat > /etc/systemd/system/dserver.service << EOF
[Unit]
Description = dserver start service
After=network.target

[Service]
Type=simple
ExecStartPre = /bin/sleep 2
ExecStart = $REAL_HOME/dserver/tool/DServer server --platform fsd-pc --mode dds --compatible
Environment="LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$REAL_HOME/dserver/lib"
User=root
Group=root
WorkingDirectory=$REAL_HOME/dserver/tool
Restart=always
RestartSec=60
StandardOutput=journal
StandardError=journal
SyslogIdentifier=dserver

# 确保以 root 权限运行
NoNewPrivileges=false
PrivateTmp=false
ProtectSystem=false
ProtectHome=false
ReadWritePaths=$REAL_HOME/dserver

[Install]
WantedBy = multi-user.target
EOF

if [ $? -ne 0 ]; then
    echo "ERROR: Failed to create dserver.service file"
    exit 1
fi

echo "Service file created. Reloading systemd daemon..."
systemctl daemon-reload
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to reload systemd daemon"
    exit 1
fi

echo "Enabling dserver.service..."
systemctl enable dserver.service
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to enable dserver.service"
    exit 1
fi

echo "Starting dserver.service..."
systemctl start dserver.service
if [ $? -ne 0 ]; then
    echo "ERROR: Failed to start dserver.service"
    echo "DEBUG: Checking service status..."
    systemctl status dserver.service
    echo "DEBUG: Checking service logs..."
    journalctl -u dserver.service --no-pager -n 20
    exit 1
fi

echo "DServer service started successfully. Checking status..."
systemctl status dserver.service --no-pager
echo "Script completed successfully!"

