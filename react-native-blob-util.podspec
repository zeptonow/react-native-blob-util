require "json"

package = JSON.parse(File.read(File.join(__dir__, "package.json")))

Pod::Spec.new do |s|
  s.name             = package['name']
  s.version          = package['version']
  s.summary          = package['description']
  s.requires_arc = true
  s.license      = 'MIT'
  s.homepage     = 'n/a'
  s.source       = { :git => "https://github.com/RonRadtke/react-native-blob-util" }
  s.author       = 'RonRadtke'
  s.source_files = 'ios/**/*.{h,m,mm,swift}'
  s.platforms       = { :ios => "11.0" }
  s.framework    = 'AssetsLibrary'

  install_modules_dependencies(s)

end
