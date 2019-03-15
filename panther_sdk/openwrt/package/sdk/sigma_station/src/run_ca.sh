#!/bin/sh

export WFA_ENV_AGENT_IPADDR=127.0.0.1
export WFA_ENV_AGENT_PORT=8000

./wfa_ca eth0.3 9000 &

