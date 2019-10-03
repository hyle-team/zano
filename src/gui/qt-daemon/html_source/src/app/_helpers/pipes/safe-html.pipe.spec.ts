import { SafeHTMLPipe } from './safe-html.pipe';

describe('SafeHTMLPipe', () => {
  it('create an instance', () => {
    const pipe = new SafeHTMLPipe();
    expect(pipe).toBeTruthy();
  });
});
