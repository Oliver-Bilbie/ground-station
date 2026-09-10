variable "region" {
  type        = string
  description = "Name of the AWS region to provision resources in"
}

variable "app_name" {
  type        = string
  description = "Short but user-friendly app name"
}

variable "base_domain" {
  type        = string
  description = "Name of the Route53 hosted zone domain"
}

variable "full_domain" {
  type        = string
  description = "Name of the domain to host the webapp"
}

variable "cert_arn" {
  type        = string
  description = "ARN of the ACM certificate for full_domain"
}
