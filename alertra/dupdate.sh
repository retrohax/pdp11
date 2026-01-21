REQ_CMD="req api.alertra.com 443 use_tls"

TMP=/tmp/dup$$

echo "Fetching device IDs (${TMP})..."
cat devices.req | $REQ_CMD | awk '{ if (length($0) >= 48) { s = substr($0, 1, 48); if (s ~ /^[0-9a-f]+$/) print s } }' > "${TMP}"
devices_count=`wc -l < "${TMP}"`
echo "Found ${devices_count} devices."
total_devices=0
failed_count=0

exec < "$TMP"
while read id; do
    total_devices=`expr $total_devices + 1`
    output=`sed 's/DEVICE_ID/'"$id"'/g' dupdate1.req | $REQ_CMD 2>/dev/null`
    shortnm=`echo "$output" | jsox ShortName`
    headers=`echo "$output" | jsox HostData | jsox headers`
    if test "$headers" = "{}"; then
        echo
        echo "Updating ${id} (${shortnm})..."
        sed 's/DEVICE_ID/'"$id"'/g' dupdate2.req | $REQ_CMD
    else
        echo
        echo "Skipping ${id} (${shortnm}), headers not empty: ${headers}"
        failed_count=`expr $failed_count + 1`
    fi
done
exec < /dev/tty

echo
echo "Processed ${total_devices} devices, ${failed_count} skipped."
echo "Removing ${TMP}..."
rm -f "${TMP}"
echo "Done."
