resource "aws_iam_role" "attach_execution_role" {
  name = "${var.app_name}_attach_execution_role"

  assume_role_policy = jsonencode({
    Version = "2012-10-17"
    Statement = [
      {
        Action = "sts:AssumeRole"
        Effect = "Allow"
        Principal = {
          Service = "lambda.amazonaws.com"
        }
      }
    ]
  })
}

resource "aws_iam_policy" "attach_execution_policy" {
  name = "${var.app_name}_attach-lambda-execution-policy"

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
          "${local.logs_arn_prefix}:log-group:/aws/lambda/${var.app_name}-attach",
          "${local.logs_arn_prefix}:log-group:/aws/lambda/${var.app_name}-attach:*",
        ]
      },
      {
        Effect = "Allow"
        Action = [
          "lambda:ListMicrovms"
        ]
        Resource = "*"
      },
      {
        Effect = "Allow"
        Action = [
          "lambda:RunMicrovm",
          "lambda:GetMicrovmImage"
        ]
        Resource = aws_lambdamicrovms_image.ground_station.arn
      },
      {
        Effect = "Allow"
        Action = [
          "lambda:GetMicrovm",
          "lambda:ResumeMicrovm",
          "lambda:CreateMicrovmAuthToken"
        ]
        Resource = aws_lambdamicrovms_image.ground_station.arn
      },
      {
        Effect   = "Allow"
        Action   = ["iam:PassRole"]
        Resource = aws_iam_role.microvm_execution_role.arn
      },
      {
        Effect = "Allow"
        Action = ["lambda:PassNetworkConnector"]
        Resource = [
          "arn:aws:lambda:${data.aws_region.current.region}:aws:network-connector:aws-network-connector:ALL_INGRESS",
          "arn:aws:lambda:${data.aws_region.current.region}:aws:network-connector:aws-network-connector:INTERNET_EGRESS",
        ]
      },
      {
        Effect   = "Allow"
        Action   = ["ssm:PutParameter"]
        Resource = aws_ssm_parameter.expire_at.arn
      }
    ]
  })
}

resource "aws_iam_role_policy_attachment" "attach_execution_policy_attachment" {
  role       = aws_iam_role.attach_execution_role.name
  policy_arn = aws_iam_policy.attach_execution_policy.arn
}

resource "aws_lambda_function" "attach" {
  function_name    = "${var.app_name}-attach"
  runtime          = "python3.13"
  role             = aws_iam_role.attach_execution_role.arn
  handler          = "attach.lambda_handler"
  timeout          = 25
  memory_size      = 256
  filename         = "${path.module}/../server/attach.zip"
  source_code_hash = filebase64sha256("${path.module}/../server/attach.zip")

  environment {
    variables = {
      MICROVM_IMAGE_IDENTIFIER   = aws_lambdamicrovms_image.ground_station.arn
      MICROVM_IMAGE_VERSION      = aws_lambdamicrovms_image.ground_station.latest_active_image_version
      MICROVM_EXECUTION_ROLE_ARN = aws_iam_role.microvm_execution_role.arn
      EXPIRE_AT_PARAMETER        = aws_ssm_parameter.expire_at.name
      LEASE_SECONDS              = "300"
      WS_PORT                    = "9001"
      TOKEN_EXPIRATION_MINUTES   = "60"
      MAX_WAIT_SECONDS           = "20"
    }
  }
}
