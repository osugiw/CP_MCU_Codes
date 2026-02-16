echo  "Flashing firmware to Groove AI Vision 2 Board on ttyACM0....."
sudo setfacl -m u:osugi_w:rw /dev/ttyACM0
python3 xmodem/xmodem_send.py --port=/dev/ttyACM0 --baudrate=921600 --protocol=xmodem --file=Seeed_SenseCraft_AI_2023_12_19T183401_402.img

# example:
# python3 xmodem/xmodem_send.py --port=/dev/ttyACM0 --baudrate=921600 --protocol=xmodem --file=we2_image_gen_local/output_case1_sec_wlcsp/output.img
