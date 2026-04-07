#!/bin/bash
#==============================================================================
# I2C-HID DTB Overlay 自動生成腳本
# 目標：在 QEMU ARM virt 上添加虛擬 I2C-HID 設備
#==============================================================================

set -e

WORK_DIR="${HOME}/qemu-i2c-hid-workspace"
OVERLAY_DIR="${WORK_DIR}/overlay"

# 顏色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_err() { echo -e "${RED}[ERR]${NC} $1"; exit 1; }

#------------------------------------------------------------------------------
# 參數
#------------------------------------------------------------------------------
I2C_BUS="i2c0"                    # I2C bus 名稱
I2C_ADDR="0x15"                   # I2C 從設備地址
HID_DESC_ADDR="0x1F"             # HID descriptor 地址
GPIO_IRQ="22"                      # GPIO 中斷號
VID="0x04F3"                      # Vendor ID
PID="0x0732"                      # Product ID
DEVICE_NAME="M487_I2C_HID"         # 設備名稱

#------------------------------------------------------------------------------
# 創建 Overlay DTB
#------------------------------------------------------------------------------
create_overlay() {
    mkdir -p "${OVERLAY_DIR}"
    
    local dts_file="${OVERLAY_DIR}/i2c-hid-overlay.dts"
    local dtb_file="${OVERLAY_DIR}/i2c-hid-overlay.dtb"
    
    log_info "創建 I2C-HID Overlay DTS..."
    
    cat > "${dts_file}" << EOF
/*
 * I2C-HID Device Tree Overlay
 * 
 * 用途：QEMU ARM virt 虛擬 I2C-HID 設備
 * 目標 VID:PID = ${VID}:${PID}
 * HID Report: Input 0x02 (64B), Output 0x03 (32B)
 */

/dts-v1/;
/plugin/;

/ {
    fragment@0 {
        target-path = "/amba/${I2C_BUS}";
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            status = "okay";

            /* 虛擬 I2C-HID 設備 */
            ${DEVICE_NAME}@${I2C_ADDR} {
                compatible = "hid-over-i2c";
                reg = <$(echo $I2C_ADDR | sed 's/0x//')>;
                hid-descr-addr = <$(echo $HID_DESC_ADDR | sed 's/0x//')>;
                
                /* 中斷：GIC SPI ${GPIO_IRQ} */
                interrupts-extended = <&gic GIC_SPI ${GPIO_IRQ} IRQ_TYPE_LEVEL_LOW>;
                
                /* 設備識別 */
                vid = <$(echo $VID | sed 's/0x//')>;
                pid = <$(echo $PID | sed 's/0x//')>;
            };
        };
    };
};
EOF

    log_info "DTS 檔案: ${dts_file}"
    
    # 檢查 dtc 是否可用
    if ! command -v dtc &> /dev/null; then
        log_info "安裝 dtc (device tree compiler)..."
        sudo apt install -y device-tree-compiler
    fi
    
    # 編譯 DTS -> DTB
    log_info "編譯 DTB..."
    dtc -I dts -O dtb -o "${dtb_file}" "${dts_file}" || log_err "編譯失敗"
    
    log_info "Overlay DTB 已產生: ${dtb_file}"
    echo ""
    log_info "使用方式：在 QEMU 啟動命令中加入 -device virt,dtb=YOUR_BASE_DTB.dtb"
    log_info "或使用 -fw_opt name=dtb/path,file=OVERLAY.dtb 載入此 overlay"
}

#------------------------------------------------------------------------------
# 生成完整 DTS（包含 I2C-HID 節點）
#------------------------------------------------------------------------------
create_full_dts() {
    local base_dtb="${1}"
    local output_dts="${OVERLAY_DIR}/virt-i2c-hid.dts"
    
    if [ -z "${base_dtb}" ]; then
        log_err "請提供 base DTB 路徑"
    fi
    
    if [ ! -f "${base_dtb}" ]; then
        log_err "Base DTB 不存在: ${base_dtb}"
    fi
    
    log_info "從 ${base_dtb} 反編譯..."
    dtc -I dtb -O dts -o "${output_dts}" "${base_dtb}" || log_err "反編譯失敗"
    
    # 在 /amba/i2c0 節點中添加內容
    # 這個比較複雜，需要用 sed 或手動編輯
    log_info "請手動編輯 ${output_dts}，在 i2c0 節點中添加："
    echo ""
    echo "    virt-hid@${I2C_ADDR} {"
    echo "        compatible = \"hid-over-i2c\";"
    echo "        reg = <${I2C_ADDR}>;"
    echo "        hid-descr-addr = <${HID_DESC_ADDR}>;"
    echo "        interrupts-extended = <&gic GIC_SPI ${GPIO_IRQ} IRQ_TYPE_LEVEL_LOW>;"
    echo "        vid = <${VID}>;"
    echo "        pid = <${PID}>;"
    echo "    };"
    echo ""
    log_info "然後重新編譯："
    echo "  dtc -I dts -O dtb -o virt-i2c-hid.dtb virt-i2c-hid.dts"
}

#------------------------------------------------------------------------------
# 主流程
#------------------------------------------------------------------------------
main() {
    local cmd="${1:-overlay}"
    
    case "${cmd}" in
        overlay)
            create_overlay
            ;;
        full)
            create_full_dts "$2"
            ;;
        *)
            echo "用法: $0 {overlay|full [base_dtb]}"
            echo ""
            echo "  overlay    - 創建 overlay DTB（推薦）"
            echo "  full [dtb] - 從 base DTB 創建完整 DTS"
            exit 1
            ;;
    esac
}

main "$@"
