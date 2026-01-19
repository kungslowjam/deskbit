python --version > log.txt 2>&1
echo "Running script..." >> log.txt
python swap_verify.py >> log.txt 2>&1
echo "Done." >> log.txt
