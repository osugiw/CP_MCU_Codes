# Flash to the device new firmware
echo  "Flashing firmware to Groove AI Vision 2 Board on ttyACM0....."
sudo setfacl -m u:osugi_w:rw /dev/ttyACM0
python3 xmodem/xmodem_send.py --port=/dev/ttyACM0 --baudrate=921600 --protocol=xmodem --file=we2_image_gen_local/output_case1_sec_wlcsp/output.img

# Launch minicom
minicom -s