FROM public.ecr.aws/lambda/microvms:al2023-minimal

RUN dnf install -y python3 python3-pip && dnf clean all

WORKDIR /app
COPY GroundStation /app/GroundStation
RUN chmod +x /app/GroundStation
COPY Satellite /app/Satellite
RUN chmod +x /app/Satellite
COPY orchestrator.py /app/orchestrator.py

EXPOSE 9001
CMD ["python3", "/app/orchestrator.py"]
