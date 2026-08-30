variable "container_name" {
  description = "Name for the Docker container"
  type        = string
  default     = "binlens"
}

variable "image_tag" {
  description = "Docker image tag to use"
  type        = string
  default     = "latest"
}

variable "host_port" {
  description = "Host port to map (0 = no mapping)"
  type        = number
  default     = 0
}