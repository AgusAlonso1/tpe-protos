#!/bin/bash

read -p "Enter your token: " TOKEN

COMMANDS=(
    "chistory - History of connections"
    "ccurrent - Current connections"
    "crecord - Record of concurrent connections"
    "bytess - Bytes sent"
    "bytesr - Bytes received"
    "bytest - Bytes total"
)

while true; do
    SELECTED=$(printf "%s\n" "${COMMANDS[@]}" | fzf --prompt="Select a command (Press ESC to exit): ")

    if [[ -z "$SELECTED" ]]; then
        echo "No command selected. Exiting."
        break
    fi

    COMMAND=$(echo "$SELECTED" | awk '{print $1}')

    echo "Running: ./manager_client $TOKEN $COMMAND"
    ./manager_client "$TOKEN" "$COMMAND"

    echo
    read -p "Press Enter to continue or type 'exit' to quit: " CONTINUE
    if [[ "$CONTINUE" == "exit" ]]; then
        break
    fi
done

