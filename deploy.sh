#!/bin/bash

set -e

echo "[INFO] Building the backend..."

cmake -B build-arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$(pwd)/cmake/zig-aarch64.cmake"
cmake --build build-arm64

pushd ./server > /dev/null || exit 1
TZ=UTC touch --no-dereference -a -m -t 198002010000.00 Dockerfile GroundStation Satellite orchestrator.py attach.py
TZ=UTC zip -q -j -X app.zip Dockerfile GroundStation Satellite orchestrator.py

rm -rf attach_build
mkdir -p attach_build/vendor
cp attach.py attach_build/
python3 -m pip install --upgrade boto3 -t attach_build/vendor
find attach_build -type d -name '__pycache__' -prune -exec rm -rf {} +
TZ=UTC find attach_build -exec touch --no-dereference -a -m -t 198002010000.00 {} +
rm -f attach.zip
(
  cd attach_build
  TZ=UTC zip -q -r -X ../attach.zip .
)
unzip -l attach.zip | grep -q 'vendor/botocore/' || {
  echo "[ERROR] attach.zip is missing vendored botocore"
  exit 1
}
TZ=UTC touch -a -m -t 198002010000.00 app.zip attach.zip
popd > /dev/null

echo "[INFO] Deploying the backend..."
pushd ./terraform > /dev/null || exit 1
terraform init -upgrade
terraform validate
terraform apply
CLOUDFRONT_DIST=$(terraform output -raw cloudfront_distribution)
BUCKET_NAME=$(terraform output -raw bucket_name)
ATTACH_ENDPOINT=$(terraform output -raw attach_endpoint)
popd > /dev/null

echo "[INFO] Backend deployed successfully"

echo "[INFO] Building the frontend..."
pushd ./dashboard > /dev/null || exit 1
sed -i "s&ATTACH_ENDPOINT_PLACEHOLDER&${ATTACH_ENDPOINT}&g" ./src/helpers/endpoints.js

npm install
npm run build

sed -i "s&${ATTACH_ENDPOINT}&ATTACH_ENDPOINT_PLACEHOLDER&g" ./src/helpers/endpoints.js

echo "[INFO] Syncing files to S3..."
aws s3 sync ./dist "s3://${BUCKET_NAME}"

echo "[INFO] Resetting CDN cache..."
aws cloudfront create-invalidation --distribution-id "$CLOUDFRONT_DIST" --paths "/*"

echo "[INFO] Frontend deployed successfully"
popd > /dev/null

echo "[INFO] Deployment completed"
