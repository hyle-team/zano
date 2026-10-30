#!/usr/bin/env bash
set -euo pipefail

# Keep RPC private and the administrative API disabled, including with a mounted
# data directory. Other ordinary daemon options can be supplied to docker run.
for arg in "$@"; do
    option="${arg%%=*}"
    case "$option" in
        --*)
            option="${option#--}"
            # Boost accepts unambiguous long-option abbreviations. Reject every
            # prefix of these names, as well as their complete spellings.
            for protected in rpc-bind-ip rpc-bind-port rpc-enable-admin-api config-file; do
                if [[ "$protected" == "$option"* ]]; then
                    echo "This image fixes RPC to loopback and disables the administrative API." >&2
                    exit 2
                fi
            done
            ;;
        -*)
            echo "Use complete --long-option names for daemon options." >&2
            exit 2
            ;;
    esac
done

mkdir -p /tmp/nginx/client /tmp/nginx/proxy
nginx -t

nginx_pid=
daemon_pid=
cleanup() {
    trap - INT TERM
    [ -z "$daemon_pid" ] || kill -TERM "$daemon_pid" 2>/dev/null || true
    [ -z "$nginx_pid" ] || kill -TERM "$nginx_pid" 2>/dev/null || true
    [ -z "$daemon_pid" ] || wait "$daemon_pid" 2>/dev/null || true
    [ -z "$nginx_pid" ] || wait "$nginx_pid" 2>/dev/null || true
}
trap 'cleanup; exit 0' INT TERM

nginx -g 'daemon off;' &
nginx_pid=$!
zanod --config-file=/dev/null --no-console --disable-upnp --log-level=0 \
    --rpc-bind-ip=127.0.0.1 --rpc-bind-port=11211 \
    --data-dir=/home/zano/.Zano "$@" &
daemon_pid=$!

if wait -n "$nginx_pid" "$daemon_pid"; then
    status=0
else
    status=$?
fi
cleanup
exit "$status"
