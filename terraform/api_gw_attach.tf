resource "aws_apigatewayv2_api" "attach_api" {
  name          = "${var.app_name} attach"
  protocol_type = "HTTP"

  cors_configuration {
    allow_headers = ["*"]
    allow_methods = ["GET", "POST", "OPTIONS"]
    allow_origins = ["*"]
  }
}

resource "aws_apigatewayv2_stage" "attach_stage" {
  api_id      = aws_apigatewayv2_api.attach_api.id
  name        = "$default"
  auto_deploy = true
}

resource "aws_apigatewayv2_route" "attach_route" {
  api_id    = aws_apigatewayv2_api.attach_api.id
  route_key = "POST /attach"

  target = "integrations/${aws_apigatewayv2_integration.attach_integration.id}"
}

resource "aws_apigatewayv2_integration" "attach_integration" {
  api_id                 = aws_apigatewayv2_api.attach_api.id
  integration_type       = "AWS_PROXY"
  integration_uri        = aws_lambda_function.attach.arn
  integration_method     = "POST"
  payload_format_version = "2.0"
  timeout_milliseconds   = 30000
}

resource "aws_lambda_permission" "attach_permission" {
  statement_id  = "${var.app_name}-AllowAPIGatewayConnect"
  action        = "lambda:InvokeFunction"
  function_name = aws_lambda_function.attach.arn
  principal     = "apigateway.amazonaws.com"
  source_arn    = "${aws_apigatewayv2_api.attach_api.execution_arn}/*"
}

output "attach_endpoint" {
  value = "${aws_apigatewayv2_api.attach_api.api_endpoint}/attach"
}
