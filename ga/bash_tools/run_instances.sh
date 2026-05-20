FILES="instances/*cgshop2024_instance.json"
SOLUTIONS="solutions"
CONFIG_FILE="config.json"
for f in $FILES
do
  echo "Running $f"
  # take action on each file. $f store current file name
  ../build/RelWithDebInfo/mpp.exe "$f" "$SOLUTIONS" "-c" "$CONFIG_FILE"
done