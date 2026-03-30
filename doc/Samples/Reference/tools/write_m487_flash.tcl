# M487 Flash Write with Progress
# Flash size: 512KB (0x80000)

set flash_size 0x80000
set chunk_size 0x10000
set total_chunks [expr {$flash_size / $chunk_size}]
set filename "C:/Users/rinry/m487_flash.bin"

echo "=== M487 Flash Write: $total_chunks chunks ==="
echo "Source: $filename"

init
reset halt

for {set i 0} {$i < $total_chunks} {incr i} {
    set offset [expr {$i * $chunk_size}]
    echo [format "Writing chunk %d/%d (offset 0x%08x)..." $i $total_chunks $offset]
    flash write_image erase $filename $offset
    echo [format "Chunk %d/%d done." $i $total_chunks]
}

echo "=== Flash write complete ==="
shutdown
