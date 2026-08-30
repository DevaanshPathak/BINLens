output "container_id" {
  description = "ID of the created container"
  value       = docker_container.binlens.id
}

output "image_name" {
  description = "Full Docker image name"
  value       = docker_image.binlens.name
}