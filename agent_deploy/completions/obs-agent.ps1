Register-ArgumentCompleter -CommandName obs-agent -ParameterName Command -ScriptBlock {
    param($wordToComplete)
    @('start', 'stop', 'restart', 'status', 'diag', 'version', 'config', 'upgrade', 'clear-cache') |
    Where-Object { $_ -like "$wordToComplete*" }
}
