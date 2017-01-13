#!/bin/bash

mkdir -p ../data/logs/ ../data/savedprops/ ../data/databases/

./dsstests --catch_system_errors=no --log_sink=stderr 2> >(tee -i error.log >&1)

