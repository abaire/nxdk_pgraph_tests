FROM ubuntu:noble

RUN apt update && apt install -y \
    bash \
    xvfb \
    fluxbox \
    tmux \
    x11vnc \
    libasound2t64 \
    libgbm1 \
    mesa-utils \
    pulseaudio \
    pulseaudio-utils \
    dbus-user-session

RUN adduser root pulse-access
RUN adduser root pulse

RUN mkdir -p /run/pulse && \
    chown pulse:pulse /run/pulse && \
    chmod 0775 /run/pulse

COPY --chmod=0444 xemu.toml /xemu/xemu.toml

COPY --chmod=0555 entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
