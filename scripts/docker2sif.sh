#!/bin/bash

source .venv/bin/activate
spython recipe Dockerfile > myimage.def
deactivate
apptainer build myimage.sif myimage.def
rm myimage.def