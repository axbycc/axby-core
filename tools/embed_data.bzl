def _embed_data_impl(ctx):
    out_h = ctx.actions.declare_file(ctx.label.name + ".h")

    args = []
    if ctx.attr.readable:
        args.append("--readable")
    args.append(out_h.path)

    input_files = []

    # ctx.attr.files is a dict: { Target -> string_symbol }
    for target, sym in ctx.attr.files.items():
        file_list = target.files.to_list()
        if len(file_list) != 1:
            fail(
                "embed_data: each entry in 'files' must produce exactly one file, "
                + "got %d for %s" % (len(file_list), target.label)
            )

        f = file_list[0]
        input_files.append(f)
        args.append(sym)
        args.append(f.path)

    ctx.actions.run(
        inputs = input_files,
        outputs = [out_h],
        executable = ctx.executable._tool,
        arguments = args,
        progress_message = "Embedding %d files into %s"
                          % (len(input_files), out_h.path),
    )

    return [DefaultInfo(files = depset([out_h]))]


embed_data = rule(
    implementation = _embed_data_impl,
    attrs = {
        "files": attr.label_keyed_string_dict(
            mandatory = True,
            allow_files = True,
        ),
        "readable": attr.bool(default = False),
        "_tool": attr.label(
            default = Label("//tools:embed_files"),
            cfg = "exec",
            executable = True,
        ),
    },
)
