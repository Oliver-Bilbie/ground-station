resource "aws_ssm_parameter" "expire_at" {
  name        = "${var.app_name}-expire_at"
  description = "Unix timestamp when the ${var.app_name} MicroVM orchestrator should stop"
  type        = "String"
  value       = "0"

  lifecycle {
    ignore_changes = [value]
  }
}
