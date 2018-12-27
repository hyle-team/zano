import { MoneyToIntPipe } from './money-to-int.pipe';

describe('MoneyToIntPipe', () => {
  it('create an instance', () => {
    const pipe = new MoneyToIntPipe();
    expect(pipe).toBeTruthy();
  });
});
