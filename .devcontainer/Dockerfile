# This Dockerfile constructs an environment for Matter development.
# It sets up ESP-IDF and ESP-Matter along with their respective dependencies.
# This includes the compilers and tools required to build Matter firmware.

# Use chip-build with the tag specified by VERSION as the base image
ARG VERSION=134
FROM ghcr.io/project-chip/chip-build:${VERSION}

# Set environment variables for ESP-IDF and tools
ENV IDF_PATH=/opt/espressif/esp-idf
ENV IDF_TOOLS_PATH=/opt/espressif/tools

# Declare the ESP-IDF version and repository details
# ESP-IDF TAG v5.4.4 (stable) as of 17.04.2026
ARG IDF_CLONE_URL=https://github.com/espressif/esp-idf.git
ARG IDF_CHECKOUT_REF=tags/v5.5.4

# Clone and setup the specified version of ESP-IDF
RUN set -x \
    && mkdir -p $IDF_PATH \
    && cd $IDF_PATH \
    && git init \
    && git remote add origin $IDF_CLONE_URL \
    && git fetch origin --depth=1 --recurse-submodules ${IDF_CHECKOUT_REF} \
    && git checkout FETCH_HEAD \
    && git submodule update --init --recursive --depth 1 \
    && ./install.sh \
    && rm -rf ~/.espressif/dist ~/.espressif/tmp ~/.espressif/tools/.tmp \
    && : # last line as a placeholder

# Setup esp-matter paths
# WORKDIR /opt/espressif
# ENV ESP_MATTER_PATH=/opt/espressif/esp-matter

# # Declare the esp-matter version and repository details: Matter 1.4, 23.05.2025
# ARG ESP_MATTER_CLONE_URL="https://github.com/espressif/esp-matter.git"
# ARG ESP_MATTER_CHECKOUT_REF="66d1ef97d820b4ea9ef861e2be85f94149ad338a"

# Clone and setup the specified version of esp-matter
# RUN set -x \
#     && mkdir -p $ESP_MATTER_PATH \
#     && cd $ESP_MATTER_PATH \
#     && git init \
#     && git remote add origin $ESP_MATTER_CLONE_URL \
#     && git fetch origin --depth=1 $ESP_MATTER_CHECKOUT_REF \
#     && git checkout FETCH_HEAD \
#     && git submodule update --init --depth 1 \
#     && cd ./connectedhomeip/connectedhomeip \
#     && ./scripts/checkout_submodules.py --platform esp32 linux --shallow \
#     && cd $IDF_PATH \
#     && . ./export.sh \
#     && cd $ESP_MATTER_PATH \
#     && ./install.sh \
#     && : # last line as a placeholder

# Install LTS node and additional packages
# RUN apt-get update && apt-get install -y nano \
# && curl -fsSL https://deb.nodesource.com/setup_lts.x | bash - \
# && apt-get install -y nodejs \
# && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y nano ssh \
&& apt-get clean && rm -rf /var/lib/apt/lists/*

# Set the locale
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Define user details
ARG USER=knx
ARG UID=1000
ARG GID=1000

# Create a group and user with specified UID and GID, and add the user to necessary groups and add uucp group for ArchLinux USB support
RUN set -eux; \
usermod -l "$USER" ubuntu; \
usermod -d "/home/$USER" -m "$USER"; \
groupmod -n "$USER" ubuntu || true; \
usermod -a -G root "$USER"; \
usermod -a -G dialout "$USER"; \
groupadd -g 987 usbaccess || true; \
usermod -a -G usbaccess "$USER"; \
chown -R "$USER:$USER" /opt/espressif;

# Update the user's bashrc to source the required environment scripts for ESP-IDF and ESP-Matter (similar to entrypoint.sh)
RUN echo "source $IDF_PATH/export.sh > /dev/null 2>&1" >> /home/$USER/.bashrc \
#&& echo "source $ESP_MATTER_PATH/export.sh > /dev/null 2>&1" >> /home/$USER/.bashrc \
&& echo "export IDF_CCACHE_ENABLE=1" >> /home/$USER/.bashrc

# Copy the entrypoint script to the specified location and set it as the container's entrypoint
COPY entrypoint.sh /opt/espressif/entrypoint.sh
ENTRYPOINT [ "/opt/espressif/entrypoint.sh" ]

# Set the default command to be executed when the container starts
CMD [ "/bin/bash" ]

# Switch to the newly created user and set the working directory to their home directory
USER ${USER}
WORKDIR /home/${USER}
