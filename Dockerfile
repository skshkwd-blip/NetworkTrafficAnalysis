FROM python:3.12-slim
RUN apt-get update && apt-get install -y --no-install-recommends g++ make && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY . .
RUN mkdir -p bin data && make
ENV HOST=0.0.0.0
CMD ["python3", "ui/server.py"]
