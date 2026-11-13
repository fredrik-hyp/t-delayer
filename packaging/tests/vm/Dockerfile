FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive PYTHONUNBUFFERED=1
RUN apt-get update && apt-get install -y --no-install-recommends \
    qemu-system-x86 qemu-utils ovmf genisoimage openssh-client curl ca-certificates python3 \
    && rm -rf /var/lib/apt/lists/*
COPY run.py attended_install.py named_installations.py /opt/vm-test/
ENTRYPOINT ["python3", "/opt/vm-test/run.py"]
