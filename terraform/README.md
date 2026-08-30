# BINLens Terraform Module

This module deploys BINLens as a Docker container for firmware inspection in CI/CD pipelines or local analysis environments.

## Usage

```hcl
module "binlens" {
  source = "./terraform"
  container_name = "binlens"
  image_tag      = "latest"
  host_port      = 0
}
```

## Requirements

- Terraform 1.6+
- Docker daemon running on the host

## Inputs

| Name | Description | Type | Default |
|------|-------------|------|---------|
| container_name | Name for the Docker container | string | `"binlens"` |
| image_tag | Docker image tag to use | string | `"latest"` |
| host_port | Host port to map (0 = no mapping) | number | `0` |

## Outputs

| Name | Description |
|------|-------------|
| container_id | ID of the created container |
| image_name | Full Docker image name |