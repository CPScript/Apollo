BINARY="distribution/apollo.bin"

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary file $BINARY not found"
    exit 1
fi

echo "=== Multiboot Header Debug ==="
echo "Binary file: $BINARY"
echo "File size: $(stat -c%s "$BINARY") bytes"
echo ""

echo "First 64 bytes (hex):"
hexdump -C -n 64 "$BINARY"
echo ""

echo "Searching for Multiboot2 magic (0xe85250d6):"
MAGIC_POS=$(xxd -p "$BINARY" | tr -d '\n' | grep -b -o "d65052e8" | head -1 | cut -d: -f1)

if [ -n "$MAGIC_POS" ]; then
    BYTE_POS=$((MAGIC_POS / 2))
    echo "Found multiboot2 magic at byte offset: $BYTE_POS"
    
    if [ $BYTE_POS -le 32768 ]; then
        echo "✓ Magic is within first 32KB (GRUB requirement)"
    else
        echo "✗ Magic is beyond 32KB - GRUB won't find it"
    fi
else
    echo "✗ Multiboot2 magic not found!"
fi

echo ""
echo "First 16 bytes as 32-bit words:"
hexdump -e '4/4 "0x%08x " "\n"' -n 16 "$BINARY"