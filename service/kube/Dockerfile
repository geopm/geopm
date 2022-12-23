FROM opensuse/leap
RUN zypper install -y systemd systemd-sysvinit curl coreutils
RUN curl -fsSL https://download.opensuse.org/repositories/hardware/15.4/repodata/repomd.xml.key > /tmp/suse-key
RUN rpm --import /tmp/suse-key
RUN zypper addrepo https://download.opensuse.org/repositories/hardware/15.4/hardware.repo
RUN zypper install -y geopm-service
