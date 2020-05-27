(
  echo "print(("
  cat "${@:1}" | \
    grep Avg | tr -d "Avg time: " | tr "\n" "+"
  echo "0) * 20 / 3600)"
) | python3
