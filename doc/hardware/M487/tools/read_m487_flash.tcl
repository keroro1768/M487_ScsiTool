# M487 Flash read with progress
# Reads 1MB (0x100000) in 64KB chunks

set flash_size 0x100000
set chunk_size 0x10000
set total_chunks [expr {$flash_size / $chunk_size}]
set filename "C:/Users/rinry/m487_full.bin"

echo "=== M487 Flash Read: $total_chunks chunks ==="

for {set i 0} {$i < $total_chunks} {incr i} {
    set offset [expr {$i * $chunk_size}]
    echo [format "Reading chunk %d/%d (offset 0x%08x, size 0x%08x)..." $i $total_chunks $offset $chunk_size]
    
    flash read_bank 0 $filename $offset $chunk_size
    
    echo [format "Chunk %d/%d done." $i $total_chunks]
}

echo "=== Flash read complete: $filename ==="
shutdown
