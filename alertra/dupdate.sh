REQ_CMD="req api.alertra.com 443"

TMP=/tmp/dup$$

cat devices.req | $REQ_CMD | awk '{ if (length($0) >= 48) { s = substr($0, 1, 48); if (s ~ /^[0-9a-f]+$/) print s } }' > "$TMP"

while read id; do
    sed 's/DEVICE_ID/'"$id"'/g' dupdate1.req | \
    $REQ_CMD
    break
done < "$TMP"

rm -f "$TMP"
