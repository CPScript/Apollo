BINARY="distribution/apollo.bin"

if [ ! -f "$BINARY" ]; then
    echo "Error: Binary file $BINARY not found"
    exit 1
fi

echo "=== ELF and Multiboot Verification ==="
echo "File: $BINARY"
echo ""

echo "ELF Header Check:"
if file "$BINARY" | grep -q "ELF"; then
    echo "✓ File is in ELF format"
    file "$BINARY"
else
    echo "✗ File is not in ELF format"
    echo "File type: $(file "$BINARY")"
fi
echo ""

if command -v readelf &> /dev/null; then
    echo "ELF Header Details:"
    readelf -h "$BINARY" | head -15
    echo ""
    
    echo "ELF Sections:"
    readelf -S "$BINARY" | grep -E "(Name|multiboot|text)"
fi

echo ""
echo "Multiboot Header Search:"

MB1_POS=$(xxd -p "$BINARY" | tr -d '\n' | grep -b -o -i "02b0ad1b" | head -1 | cut -d: -f1)
if [ -n "$MB1_POS" ]; then
    BYTE_POS=$((MB1_POS / 2))
    echo "✓ Multiboot1 magic found at byte offset: $BYTE_POS"
else
    echo "✗ Multiboot1 magic not found"
fi

MB2_POS=$(xxd -p "$BINARY" | tr -d '\n' | grep -b -o -i "d65052e8" | head -1 | cut -d: -f1)
if [ -n "$MB2_POS" ]; then
    BYTE_POS=$((MB2_POS / 2))
    echo "✓ Multiboot2 magic found at byte offset: $BYTE_POS"
    
    if [ $BYTE_POS -le 32768 ]; then
        echo "✓ Multiboot header is within first 32KB"
    else
        echo "✗ Multiboot header is beyond 32KB limit"
    fi
else
    echo "✗ Multiboot2 magic not found"
fi

echo ""
echo "Entry Point Check:"
if command -v readelf &> /dev/null; then
    ENTRY=$(readelf -h "$BINARY" | grep "Entry point" | awk '{print $4}')
    echo "Entry point address: $ENTRY"
fi