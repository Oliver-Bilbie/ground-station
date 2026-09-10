data "aws_region" "current" {}
data "aws_caller_identity" "current" {}

locals {
  logs_arn_prefix   = "arn:aws:logs:${data.aws_region.current.region}:${data.aws_caller_identity.current.account_id}"
  lambda_arn_prefix = "arn:aws:lambda:${data.aws_region.current.region}:${data.aws_caller_identity.current.account_id}"
}

resource "aws_s3_bucket" "microvm-artifacts" {
  bucket = "${var.app_name}-microvm-artifacts"
}

resource "aws_s3_bucket_public_access_block" "microvm-artifacts-public-access" {
  bucket                  = aws_s3_bucket.microvm-artifacts.id
  block_public_acls       = true
  block_public_policy     = true
  ignore_public_acls      = true
  restrict_public_buckets = true
}

resource "aws_s3_object" "microvm-artifact" {
  bucket = aws_s3_bucket.microvm-artifacts.id
  key    = "app-${filemd5("${path.module}/../server/app.zip")}.zip"
  source = "${path.module}/../server/app.zip"
  etag   = filemd5("${path.module}/../server/app.zip")
}

resource "aws_iam_role" "microvm_build_role" {
  name = "${var.app_name}_microvm_build_role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = ["sts:AssumeRole", "sts:TagSession"]
        Effect = "Allow"
        Principal = {
          Service = "lambda.amazonaws.com"
        }
      }
    ]
  })
}

resource "aws_iam_policy" "microvm_build_policy" {
  name = "${var.app_name}_microvm-build-policy"

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect   = "Allow"
        Action   = ["s3:GetObject"]
        Resource = aws_s3_object.microvm-artifact.arn
      },
      {
        Effect = "Allow"
        Action = [
          "logs:CreateLogGroup",
          "logs:CreateLogStream",
          "logs:PutLogEvents"
        ]
        Resource = [
          "${local.logs_arn_prefix}:log-group:*",
          "${local.logs_arn_prefix}:log-group:*:*",
        ]
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "microvm_build_policy_attachment" {
  role       = aws_iam_role.microvm_build_role.name
  policy_arn = aws_iam_policy.microvm_build_policy.arn
}

resource "aws_iam_role" "microvm_execution_role" {
  name = "${var.app_name}_microvm_execution_role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = ["sts:AssumeRole", "sts:TagSession"]
        Effect = "Allow"
        Principal = {
          Service = "lambda.amazonaws.com"
        }
      }
    ]
  })
}

resource "aws_iam_policy" "microvm_execution_policy" {
  name = "${var.app_name}_microvm-execution-policy"

  policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Effect = "Allow"
        Action = [
          "logs:CreateLogGroup",
          "logs:CreateLogStream",
          "logs:PutLogEvents"
        ]
        Resource = [
          "${local.logs_arn_prefix}:log-group:*",
          "${local.logs_arn_prefix}:log-group:*:*",
        ]
      },
      {
        Effect   = "Allow"
        Action   = ["ssm:GetParameter"]
        Resource = aws_ssm_parameter.expire_at.arn
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "microvm_execution_policy_attachment" {
  role       = aws_iam_role.microvm_execution_role.name
  policy_arn = aws_iam_policy.microvm_execution_policy.arn
}

resource "aws_lambdamicrovms_image" "ground_station" {
  name           = var.app_name
  description    = "Ground station orchestrator MicroVM image"
  base_image_arn = "arn:aws:lambda:${data.aws_region.current.region}:aws:microvm-image:al2023-1"
  build_role_arn = aws_iam_role.microvm_build_role.arn

  environment_variables = {
    EXPIRE_AT_PARAMETER = aws_ssm_parameter.expire_at.name
    AWS_DEFAULT_REGION  = data.aws_region.current.region
  }

  code_artifact {
    uri = "s3://${aws_s3_bucket.microvm-artifacts.bucket}/${aws_s3_object.microvm-artifact.key}"
  }

  cpu_configuration {
    architecture = "ARM_64"
  }

  tags = {
    Name = var.app_name
  }

  depends_on = [aws_iam_role_policy_attachment.microvm_build_policy_attachment]
}
