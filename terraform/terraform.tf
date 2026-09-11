terraform {
  backend "s3" {
    region       = "us-east-1"
    bucket       = "obilbie-tf-state-bucket"
    key          = "ground-station/terraform.tfstate"
    use_lockfile = true
    encrypt      = true
  }

  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = ">= 6.61.0"
    }
  }
}

provider "aws" {
  region = var.region

  default_tags {
    tags = {
      Project = var.app_name
    }
  }
}
