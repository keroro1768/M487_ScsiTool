#!/bin/bash
#==============================================================================
# QEMU ARM virt + I2C-HID 環境設定腳本
# 用於：FWUPD 測試、虛擬 HID-over-I2C 設備開發
# 目標：x86_64 Ubuntu Host → ARM64 VM (virt board)
#==============================================================================

set -e

#------------------------------------------------------------------------------
# 參數設定（可根據環境調整）
#------------------------------------------------------------------------------
ARM64_IMAGE="ubuntu-24.04-server-cloudimg-arm64.img"
ARM64_IMAGE_URL="https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-arm64.img"
IMAGE_SIZE="10G"
KERNEL_URL="https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-arm64-kernel-5"
INITRD_URL="https://cloud-images.ubuntu.com/release/24.04/release/ubuntu-24.04-server-cloudimg-arm64-initrd-generic"

WORK_DIR="${HOME}/qemu-i2c-hid-workspace"
DTB_DIR="${WORK_DIR}/dtb"
SSH_KEY="${WORK_DIR}/id_rsa"

#------------------------------------------------------------------------------
# 顏色輸出
#------------------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_err() { echo -e "${RED}[ERR]${NC} $1"; exit 1; }

#------------------------------------------------------------------------------
# 檢查依賴
#------------------------------------------------------------------------------
check_dependencies() {
    log_info "檢查依賴..."
    
    # 檢查 qemu-system-aarch64
    if ! command -v qemu-system-aarch64 &> /dev/null; then
        log_err "qemu-system-aarch64 未安裝。請執行：sudo apt install qemu-system-arm"
    fi
    
    # 檢查 cloud-localds (cloud-init 工具)
    if ! command -v cloud-localds &> /dev/null; then
        log_info "cloud-localds 未安裝，安裝中..."
        sudo apt install -y cloud-image-utils
    fi
    
    # 檢查 git
    if ! command -v git &> /dev/null; then
        sudo apt install -y git
    fi
    
    log_info "依賴檢查完成"
}

#------------------------------------------------------------------------------
# 建立工作目錄
#------------------------------------------------------------------------------
setup_workspace() {
    log_info "建立工作目錄: ${WORK_DIR}"
    mkdir -p "${WORK_DIR}"
    mkdir -p "${DTB_DIR}"
    
    # 產生 SSH key（如果不存在）
    if [ ! -f "${SSH_KEY}" ]; then
        log_info "產生 SSH key..."
        ssh-keygen -t rsa -b 4096 -f "${SSH_KEY}" -N ""
    fi
}

#------------------------------------------------------------------------------
# 下載 Ubuntu ARM64 Cloud Image
#------------------------------------------------------------------------------
download_image() {
    local img_path="${WORK_DIR}/${ARM64_IMAGE}"
    
    if [ -f "${img_path}" ]; then
        log_info "Image 已存在: ${img_path}"
        return
    fi
    
    log_info "下載 Ubuntu 24.04 ARM64 Cloud Image..."
    wget -O "${img_path}" "${ARM64_IMAGE_URL}" || log_err "下載失敗"
    
    log_info "調整 image 大小..."
    qemu-img resize "${img_path}" "${IMAGE_SIZE}" || log_err "調整大小失敗"
}

#------------------------------------------------------------------------------
# 產生 Cloud-Init ISO（用於 VM 初始設定）
#------------------------------------------------------------------------------
generate_cloud_init() {
    local img_path="${WORK_DIR}/${ARM64_IMAGE}"
    local cidata_path="${WORK_DIR}/cidata.iso"
    local user_data="${WORK_DIR}/user-data"
    local meta_data="${WORK_DIR}/meta-data"
    
    log_info "產生 cloud-init 設定..."
    
    # user-data
    cat > "${user_data}" << 'EOF'
#cloud-config
hostname: qemu-arm-i2c-hid
manage_etc_hosts: true
users:
  - name: ubuntu
    sudo: ["ALL=(ALL) NOPASSWD:ALL"]
    ssh_authorized_keys:
      - PLACEHOLDER_SSH_KEY
EOF
    
    # 替換 SSH key
    local ssh_key_content=$(cat "${SSH_KEY}.pub")
    sed -i "s|PLACEHOLDER_SSH_KEY|${ssh_key_content}|" "${user_data}"
    
    # meta-data
    cat > "${meta_data}" << 'EOF'
instance-id: qemu-arm-i2c-hid-01
local-hostname: qemu-arm-i2c-hid
EOF
    
    # 產生 ISO
    log_info "產生 cidata ISO..."
    cloud-localds "${cidata_path}" "${user_data}" "${meta_data}"
    
    log_info "Cloud-init ISO 已產生: ${cidata_path}"
}

#------------------------------------------------------------------------------
# 建立自定義 DTB（I2C-HID 設備節點）
#------------------------------------------------------------------------------
create_custom_dtb() {
    log_info "建立自定義 DTB for I2C-HID..."
    
    # 首先需要獲取預設 DTB
    # QEMU ARM virt 使用的 DTB 可以從 linux-image 中提取
    # 或者從 QEMU 官網下載
    
    local default_dtb="${WORK_DIR}/virt-gicv3.dtb"
    local custom_dtb="${DTB_DIR}/virt-i2c-hid.dtb"
    local dts_file="${DTB_DIR}/virt-i2c-hid.dts"
    
    # 方法1：從 QEMU 內建 DTB 提取（如果 QEMU 有提供）
    # QEMU 的 ARM virt machine 會自動生成 DTB，我們需要修改它
    
    # 方法2：使用 dtc 從現有 DTB 反編譯後修改
    # 下載標準 DTB
    log_info "下載預設 DTB..."
    local dtb_url="https://snapshots.linaro.org/components/kernel/leg-virt-tcgc/2024.03.1-ACME/arm64/armv8/Image.gz"
    
    # 實際上，我們需要一個更好的方法
    # 使用 QEMU 內建 DTB 或從 linux-firmware 获取
    
    # 最簡單的方法：使用 QEMU 的 -machine dump-dt=dump.dtb 導出然後修改
    log_warn "請稍後手動執行以下命令獲取 DTB："
    echo "  qemu-system-aarch64 -machine virt,dump-dt=virt.dtb ..."
    echo "  dtc -I dtb -O dts virt.dtb -o virt.dts"
    echo "  # 編輯 virt.dts 添加 I2C-HID 節點"
    echo "  dtc -I dts -O dtb virt-modified.dtb virt.dts"
    
    # 創建一個示例 DTS 文件供參考
    create_sample_dts "${dts_file}"
    
    log_info "示例 DTS 已創建: ${dts_file}"
    log_info "請參考此文件手動生成 DTB，或運行自動腳本"
}

#------------------------------------------------------------------------------
# 創建示例 DTS 文件（I2C-HID 設備節點）
#------------------------------------------------------------------------------
create_sample_dts() {
    local dts_file="$1"
    
    cat > "${dts_file}" << 'EOF'
/*
 * QEMU ARM virt Machine - Custom DTB with I2C-HID Device
 * 
 * 使用方式：
 *   1. 先導出 QEMU 預設 DTB：
 *      qemu-system-aarch64 -machine virt,dump-dt=virt.dtb ...
 *   
 *   2. 反編譯：
 *      dtc -I dtb -O dts virt.dtb -o virt.dts
 *   
 *   3. 合併此文件中 i2c0 節點到 virt.dts
 *   
 *   4. 重新編譯：
 *      dtc -I dts -O dtb virt-modified.dtb virt.dts
 */

/ {
    /* 保留原有的 reserved-memory、firmware 等節點 */
    
    /* I2C bus 節點 - 添加到 /amba 之下 */
    i2c0 {
        compatible = "arm,versatile-i2c";
        reg = <0x00180000 0x1000>;
        status = "okay";
        
        /* 虛擬 I2C-HID 觸控設備（用於 FWUPD 測試） */
        virt-hid@15 {
            compatible = "elan,em欺詐tep5510";  /* 欺騙驅動程式 */
            reg = <0x15>;                        /* I2C 地址 0x15 */
            hid-descr-addr = <0x1F>;             /* HID descriptor 寄存器 */
            
            /* 中斷設定（GPIO 22） */
            interrupts-extended = <&gic GIC_SPI 22 IRQ_TYPE_LEVEL_LOW>;
            
            /* 設備識別 */
            vid = <0x04F3>;
            pid = <0x0732>;
            
            /* Elan 特有屬性（可選） */
            elan,module-id = <0x1234>;
        };
    };
};

/* 
 * 另一種方式：直接使用 i2c-hid-overlay.dts 作為 overlay
 * 這種方式不需要修改完整的 DTB，只添加 overlay 即可
 */

/ {
    fragment@0 {
        target = <&i2c0>;
        __overlay__ {
            status = "okay";
            #address-cells = <1>;
            #size-cells = <0>;
            
            virt-hid@15 {
                compatible = "hid-over-i2c";
                reg = <0x15>;
                hid-descr-addr = <0x1F>;
                interrupts-extended = <&gic GIC_SPI 22 IRQ_TYPE_LEVEL_LOW>;
                vid = <0x04F3>;
                pid = <0x0732>;
            };
        };
    };
};
EOF
}

#------------------------------------------------------------------------------
# 啟動 QEMU VM
#------------------------------------------------------------------------------
start_vm() {
    local img_path="${WORK_DIR}/${ARM64_IMAGE}"
    local cidata_path="${WORK_DIR}/cidata.iso}"
    local dtb_path="${DTB_DIR}/virt-i2c-hid.dtb"
    local ssh_key="${SSH_KEY}"
    
    # 檢查必要檔案
    [ -f "${img_path}" ] || log_err "Image 不存在: ${img_path}"
    
    log_info "啟動 QEMU ARM64 VM..."
    log_info "登入後執行：sudo apt update && sudo apt install -y linux-image-$(uname -r)"
    
    # QEMU 啟動命令
    # -machine virt: ARM virt 板
    # -cpu cortex-a57: ARM Cortex-A57 CPU
    # -m 4G: 4GB RAM
    # -nic user,hostfwd: 網路轉發（SSH 2222->22）
    # -drive: Ubuntu ARM64 cloud image
    # -cdrom: cloud-init ISO
    
    # 如果有自定義 DTB，加上 -dtb 參數
    local dtb_arg=""
    if [ -f "${dtb_path}" ]; then
        dtb_arg="-dtb ${dtb_path}"
    else
        log_warn "使用 QEMU 內建 DTB（無自定義 I2C-HID）"
    fi
    
    # SSH key 路徑替換
    local ssh_key_arg=""
    if [ -f "${ssh_key}" ]; then
        ssh_key_arg=",key=${ssh_key}"
    fi
    
    cat << 'EOF'

================================================================================
  QEMU ARM64 VM 啟動命令
================================================================================
EOF

    echo ""
    echo "  qemu-system-aarch64 \\"
    echo "    -machine virt,virtualization=on,gic-version=3 \\"
    echo "    -cpu cortex-a72 \\"
    echo "    -m 4G \\"
    echo "    -nic user,hostfwd=tcp::2222-:22${ssh_key_arg} \\"
    echo "    -drive if=virtio,file=${img_path},format=qcow2 \\"
    echo "    -cdrom ${cidata_path} \\"
    echo "    -dtb ${dtb_path} \\"
    echo "    -nographic"
    echo ""
    echo "================================================================================"
    echo "  VM 啟動後可通過以下方式登入："
    echo "    ssh -p 2222 -i ${ssh_key} ubuntu@localhost"
    echo "================================================================================"
    echo ""
    
    # 實際執行（可註解掉先做 dry-run）
    # qemu-system-aarch64 \
    #     -machine virt,virtualization=on,gic-version=3 \
    #     -cpu cortex-a72 \
    #     -m 4G \
    #     -nic user,hostfwd=tcp::2222-:22 \
    #     -drive if=virtio,file="${img_path}",format=qcow2 \
    #     -cdrom "${cidata_path}" \
    #     ${dtb_arg} \
    #     -nographic
}

#------------------------------------------------------------------------------
# 驗證 VM 內的 I2C-HID（登入 VM 後執行）
#------------------------------------------------------------------------------
verify_i2c_hid() {
    cat << 'EOF'

================================================================================
  VM 內驗證步驟（SSH 登入後執行）
================================================================================

# 1. 確認 i2c-dev 可用
ls /dev/i2c-* || echo "i2c-dev 未加載"
sudo modprobe i2c-dev
ls /dev/i2c-*

# 2. 確認 i2c-hid 核心模組
lsmod | grep i2c_hid
sudo modprobe i2c-hid

# 3. 檢查 sysfs 中的 I2C 設備
ls /sys/bus/i2c/devices/

# 4. 檢查 i2c-hid 匯流排
ls /sys/bus/i2c-hid/

# 5. 如果有 I2C-HID 設備，檢查 HID raw 設備
ls /dev/hidraw*

# 6. 查看 FWUPD 設備
sudo fwupdmgr get-devices

# 7. 如果 VM 有網路，安裝並測試 FWUPD
sudo apt install -y fwupd
sudo systemctl enable --now fwupd
sudo fwupdmgr refresh
sudo fwupdmgr get-updates

================================================================================
EOF
}

#------------------------------------------------------------------------------
# 清理
#------------------------------------------------------------------------------
cleanup() {
    log_info "清理..."
    rm -rf "${WORK_DIR:?}"/*.img "${WORK_DIR:?}"/*.iso 2>/dev/null || true
    log_info "完成"
}

#------------------------------------------------------------------------------
# 主流程
#------------------------------------------------------------------------------
main() {
    local cmd="${1:-all}"
    
    case "${cmd}" in
        all)
            check_dependencies
            setup_workspace
            download_image
            generate_cloud_init
            create_custom_dtb
            start_vm
            verify_i2c_hid
            ;;
        deps)
            check_dependencies
            ;;
        workspace)
            setup_workspace
            ;;
        download)
            setup_workspace
            download_image
            ;;
        cloud-init)
            generate_cloud_init
            ;;
        dtb)
            create_custom_dtb
            ;;
        start)
            start_vm
            ;;
        verify)
            verify_i2c_hid
            ;;
        clean)
            cleanup
            ;;
        *)
            echo "用法: $0 {all|deps|workspace|download|cloud-init|dtb|start|verify|clean}"
            echo ""
            echo "  all       - 執行全部步驟"
            echo "  deps      - 檢查依賴"
            echo "  workspace - 建立工作目錄"
            echo "  download  - 下載 Ubuntu ARM64 image"
            echo "  cloud-init- 產生 cloud-init ISO"
            echo "  dtb       - 建立自定義 DTB"
            echo "  start     - 啟動 VM"
            echo "  verify    - 顯示 VM 內驗證步驟"
            echo "  clean     - 清理工作目錄"
            exit 1
            ;;
    esac
}

main "$@"
