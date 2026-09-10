terraform {
  backend "s3" {
    region       = "eu-west-1"
    bucket       = "oliver-bilbie-tf-state-bucket"
    key          = "ground-station/terraform.tfstate"
    use_lockfile = true
    encrypt      = true
  }
}
