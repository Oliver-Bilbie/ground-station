import json
import os
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "vendor"))
import boto3

REGION = os.environ.get("AWS_REGION", os.environ.get("AWS_DEFAULT_REGION", "us-east-1"))
IMAGE_ID = os.environ["MICROVM_IMAGE_IDENTIFIER"]
IMAGE_VERSION = os.environ.get("MICROVM_IMAGE_VERSION")
EXEC_ROLE_ARN = os.environ.get("MICROVM_EXECUTION_ROLE_ARN")
WS_PORT = int(os.environ.get("WS_PORT", "9001"))
TOKEN_MINUTES = int(os.environ.get("TOKEN_EXPIRATION_MINUTES", "60"))
MAX_WAIT_S = int(os.environ.get("MAX_WAIT_SECONDS", "60"))
EXPIRE_AT_PARAMETER = os.environ["EXPIRE_AT_PARAMETER"]
LEASE_SECONDS = int(os.environ.get("LEASE_SECONDS", "300"))

INGRESS = (
    f"arn:aws:lambda:{REGION}:aws:network-connector:aws-network-connector:ALL_INGRESS"
)
EGRESS = f"arn:aws:lambda:{REGION}:aws:network-connector:aws-network-connector:INTERNET_EGRESS"

USABLE = {"PENDING", "RUNNING", "SUSPENDING", "SUSPENDED"}

client = boto3.client("lambda-microvms")
ssm = boto3.client("ssm")


def lambda_handler(event, context):
    try:
        refresh_lease()
        vm = find_existing() or start_new()
        vm = wait_until_running(vm["microvmId"])
        token = mint_token(vm["microvmId"])
        return respond(200, connection_payload(vm, token))
    except Exception as e:
        print(e)
        return respond(500, {"error": "attach failed"})


def refresh_lease():
    ssm.put_parameter(
        Name=EXPIRE_AT_PARAMETER,
        Value=str(int(time.time()) + LEASE_SECONDS),
        Type="String",
        Overwrite=True,
    )


def _started_at(item):
    value = item.get("startedAt")
    if value is None:
        return 0
    if hasattr(value, "timestamp"):
        return value.timestamp()
    return value


def find_existing():
    kwargs = {"imageIdentifier": IMAGE_ID, "maxResults": 50}
    if IMAGE_VERSION:
        kwargs["imageVersion"] = IMAGE_VERSION

    items = []
    while True:
        resp = client.list_microvms(**kwargs)
        items.extend(resp.get("items") or resp.get("Items") or [])
        token = resp.get("nextToken")
        if not token:
            break
        kwargs["nextToken"] = token

    live = [i for i in items if i.get("state") in USABLE]
    if not live:
        return None

    live.sort(key=lambda i: _started_at(i))
    chosen = live[0]
    return client.get_microvm(microvmIdentifier=chosen["microvmId"])


def start_new():
    args = {
        "imageIdentifier": IMAGE_ID,
        "ingressNetworkConnectors": [INGRESS],
        "egressNetworkConnectors": [EGRESS],
        "maximumDurationInSeconds": 3600,
        "idlePolicy": {
            "autoResumeEnabled": True,
            "maxIdleDurationSeconds": 300,
            "suspendedDurationSeconds": 0,
        },
    }
    if IMAGE_VERSION:
        args["imageVersion"] = IMAGE_VERSION
    if EXEC_ROLE_ARN:
        args["executionRoleArn"] = EXEC_ROLE_ARN

    return client.run_microvm(**args)


def wait_until_running(microvm_id):
    deadline = time.time() + MAX_WAIT_S
    last = None
    while time.time() < deadline:
        last = client.get_microvm(microvmIdentifier=microvm_id)
        state = last["state"]

        if state == "RUNNING":
            return last
        if state == "SUSPENDED":
            client.resume_microvm(microvmIdentifier=microvm_id)
        if state in ("TERMINATING", "TERMINATED"):
            raise RuntimeError(f"MicroVM ended before it was usable: {state}")

        time.sleep(1)

    raise TimeoutError(
        f"MicroVM not RUNNING after {MAX_WAIT_S}s; last state={last and last.get('state')}"
    )


def mint_token(microvm_id):
    resp = client.create_microvm_auth_token(
        microvmIdentifier=microvm_id,
        expirationInMinutes=TOKEN_MINUTES,
        allowedPorts=[{"port": WS_PORT}],
    )
    return resp["authToken"]["X-aws-proxy-auth"]


def connection_payload(vm, token):
    host = vm["endpoint"].replace("https://", "").rstrip("/")
    return {
        "microvmId": vm["microvmId"],
        "state": vm["state"],
        "endpoint": host,
        "port": WS_PORT,
        "token": token,
        "wsUrl": f"wss://{host}/",
        "protocols": [
            "lambda-microvms",
            f"lambda-microvms.authentication.{token}",
            f"lambda-microvms.port.{WS_PORT}",
        ],
    }


def respond(status, body):
    return {
        "statusCode": status,
        "headers": {
            "Content-Type": "application/json",
            "Access-Control-Allow-Origin": "*",
            "Access-Control-Allow-Headers": "*",
        },
        "body": json.dumps(body),
    }
